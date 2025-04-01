#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include "assembly.h"
#include "assembly_library.h"

pthread_t threads[7];

assembly_line_t line;

sem_t sem_signal;
sem_t sem_arm;
sem_t sem_restart;
sem_t sem_watchdog;

timer_t watchdog_timer;

atomic_int shutdown_flag = 0;
atomic_int watchdog_flag = 0;

int PET_TIME = (int)(BELT_PERIOD*4);

typedef struct {
    int part;
    side_t side;
    int position;
} arm_task_t;

arm_task_t arm[7] = {
    {PART_FRAME, LEFT, 1},
    {PART_ENGINE,LEFT, 2},
    {PART_WHEELS,RIGHT, 2},
    {PART_BODY,LEFT, 3},
    {PART_DOORS,RIGHT, 4},
    {PART_LIGHTS,LEFT, 4},
    {PART_WINDOWS,RIGHT, 5}
};

const char* part_to_string(int part) {
    switch (part) {
        case PART_FRAME: return "Frame";
        case PART_ENGINE: return "Engine";
        case PART_WHEELS: return "Wheels";
        case PART_BODY: return "Body";
        case PART_DOORS: return "Doors";
        case PART_LIGHTS: return "Lights";
        case PART_WINDOWS: return "Windows";
        default: return "Unknown";
    }
}

void* get_currenttime(struct timespec* ts) {
    if (clock_gettime(CLOCK_REALTIME, ts) != 0) {
        perror("clock_gettime failed");
    }
}

void* arm_task_loop(void* arg) {
    arm_task_t* task = (arm_task_t*)arg;
    struct timespec ts; 

    while(!shutdown_flag){
        if(watchdog_flag) { 
            sem_post(&sem_watchdog); 
            printf_green("Arm for %s ready\n", part_to_string(task->part));
        } // tell the watchdog that the arm is ready
        sem_wait(&sem_arm); // wait to start

        if(shutdown_flag) break; // check if shutdown is requested

        get_currenttime(&ts);
        delay_until(&ts, (BELT_PERIOD * task->position + 10));

        while(!shutdown_flag && !watchdog_flag){
            get_currenttime(&ts);
            trigger_arm(line, task->side, task->position); // install the part
            pet_watchdog(watchdog_timer, PET_TIME); // pet the watchdog
            delay_until(&ts, BELT_PERIOD*7); // wait for the right time
        }
    }

    printf_red("Arm for %s shutdown\n", part_to_string(task->part));
    free(task);
    return NULL;
}

void setup_arms(){
    for (int i = 0; i < 7; i++) {
        setup_arm(line, arm[i].part, arm[i].side, arm[i].position);
    }
}

void create_arm_task(){
    for (int i = 0; i < 7; i++) {
        arm_task_t* task = malloc(sizeof(arm_task_t));
        *task = arm[i];
        pthread_create(&threads[i], NULL, arm_task_loop, task);
    }
}

void* handle_show_stats(void* arg) {
    while(!shutdown_flag){
        sem_wait(&sem_signal);
        print_assembly_stats(line);
    }
    return NULL;
}

void handler(int signum) {
    if(signum == SIGUSR1) { // new terminal "ps aux | grep nom_du_programme" -> kill -SIGUSR1 <pid>
        sem_post(&sem_signal);
    }
    else if (signum == SIGINT) { // CTRL+C
        printf("Signal %d received. Shutting down...\n", signum);
        shutdown_flag = 1;
        shutdown_assembly(line); // shutdown the assembly line
        if(watchdog_flag) {
            for (int i = 0; i < 7; i++) { sem_post(&sem_arm); }
            for(int i = 0; i < 7; i++) { sem_post(&sem_watchdog); }
        }
    }
}

void start(){
    for (int i = 0; i < 7; i++) { sem_post(&sem_arm); } // Launch the arms
    run_assembly(line); // run the assembly line
}

void watchdog_handler(union sigval arg) {
    if(!watchdog_flag && !shutdown_flag){
        printf_red("Watchdog !\n");
        watchdog_flag = 1;  
        shutdown_assembly(line); // shutdown the assembly line
    }
}

// MAIN

int main(){
    //setup
    init_assembly_line(&line);
    setup_arms();
    printf("Setup done\n");

    // setup semaphores
    sem_init(&sem_arm, 0, 0); // setup waiting semaphore for arms
    sem_init(&sem_signal, 0, 0); // setup semaphore for signal
    sem_init(&sem_watchdog, 0, 0); // setup semaphore for watchdog

    // setup signal handler
    pthread_t thread;
    pthread_create(&thread, NULL, handle_show_stats, NULL);
    handle_signal(SIGUSR1, handler);
    handle_signal(SIGINT, handler);

    // setup watchdog
    watchdog_timer = watchdog_function(watchdog_handler); 

    create_arm_task();

    while(!shutdown_flag){
        if(watchdog_flag) {
            printf_green("Wait for all arms to be ready\n");
            for(int i = 0; i < 7; i++){ sem_wait(&sem_watchdog); } // wait for all arms to be ready
            watchdog_flag = 0;
            printf_green("Restarting...\n");
        }

        if(shutdown_flag) break; // check if shutdown is requested

        pet_watchdog(watchdog_timer, PET_TIME); // pet the watchdog

        start();
    }

    for(int i = 0; i < 7; i++){ // wait all arm shutdown
        pthread_join(threads[i], NULL);
    }

    print_assembly_stats(line);
    free_assembly_line(&line);

    return 0;
}