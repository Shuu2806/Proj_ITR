#include <stdio.h>
#include <stdlib.h>

#include "assembly.h"
#include "assembly.c"

assembly_line_t line;

typedef struct {
    side_t side;
    int position;
} arm_task_t;

void* install_part(void* arg) {
    arm_task_t* task = (arm_task_t*)arg;

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        perror("clock_gettime failed");
        return NULL;
    }

    sleep_until(&ts, (BELT_PERIOD * task->position + 50));

    while (1) { 
        if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
            perror("clock_gettime failed");
            return NULL;
        }

        error_t error = trigger_arm(line, task->side, task->position);

        sleep_until(&ts, BELT_PERIOD*(MAX_POSITION-2));
    }

    return NULL;
}

void start_installation_threads() {
    pthread_t threads[7];
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

    // launch thread
    start_installation_threads();

    // run
    run_assembly(line);

    print_assembly_stats(line);
    shutdown_assembly(line);
    free_assembly_line(&line);
    return 0;
}