#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "assembly.h"
#include "assembly.c"
#include "assembly_library.h"
#include "assembly_library.c"

assembly_line_t line;
atomic_int shutdown_flag = 0;
pthread_t threads[7];

typedef struct {
    side_t side;
    int position;
} arm_task_t;

void* install_part(void* arg) {
    arm_task_t* task = (arm_task_t*)arg;

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        perror("clock_gettime failed");
        free(task);
        return NULL;
    }

    sleep_until(&ts, (BELT_PERIOD * task->position + 50));

    while (!shutdown_flag) { 
        if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
            perror("clock_gettime failed");
            return NULL;
        }

        error_t error = trigger_arm(line, task->side, task->position);

        sleep_until(&ts, BELT_PERIOD*(MAX_POSITION-2));
    }

    printf("Thread for position %d | side %d exiting.\n", task->position, task->side);
    free(task);
    return NULL;
}

void start_installation_threads() {
    arm_task_t tasks[7] = {
        {LEFT, 1},
        {LEFT, 2},
        {RIGHT, 2},
        {LEFT, 3},
        {RIGHT, 4},
        {LEFT, 4},
        {RIGHT, 5}
    };

    for (int i = 0; i < 7; i++) {
        arm_task_t* task = malloc(sizeof(arm_task_t));
        *task = tasks[i];
        pthread_create(&threads[i], NULL, install_part, task);
        printf("thread %d created\n", i);
    }
}

sem_t sem;

void* handle_show_stats(void* arg) {
    while(!shutdown_flag){
        sem_wait(&sem);
        print_assembly_stats(line);
    }
    return NULL;
}

void handler(int signum) {
    if(signum == SIGUSR1) {
        sem_post(&sem);
    }
    else if (signum == SIGINT) { // CTRL+C
        printf("Signal %d received. Shutting down...\n", signum);
        shutdown_flag = 1;
    
        // attends que les threads se termine
        for (int i = 0; i < 7; i++) {
            pthread_join(threads[i], NULL);
        }
    
        shutdown_assembly(line); 
        free_assembly_line(&line);
        printf("Signal %d handled. Resources released.\n", signum);
    }
}

int main(){
    // setup
    init_assembly_line(&line);
    setup_arm(line, PART_FRAME, LEFT, 1);
    setup_arm(line, PART_ENGINE, LEFT, 2);
    setup_arm(line, PART_WHEELS, RIGHT, 2);
    setup_arm(line, PART_BODY, LEFT, 3);
    setup_arm(line, PART_DOORS, RIGHT, 4);
    setup_arm(line, PART_LIGHTS, LEFT, 4);
    setup_arm(line, PART_WINDOWS, RIGHT, 5);

    // setup signal
    sem_init(&sem, 0, 0);

    pthread_t thread;
    pthread_create(&thread, NULL, handle_show_stats, NULL);
    handle_signal(SIGUSR1, handler);
    handle_signal(SIGINT, handler);

    // launch thread
    start_installation_threads();

    // run
    run_assembly(line);

    pthread_join(thread, NULL);
    return 0;
}