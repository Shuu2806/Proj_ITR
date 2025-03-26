
#include "assembly.h"
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>

typedef struct {
    unsigned int status;
} car_t;
typedef struct {
    unsigned long long int built_cars;
    unsigned long long int failed_cars;
    unsigned long long int starts;
} stats_t;
typedef struct {
    unsigned int belt_position;
    part_t arms[MAX_POSITION*2];
    unsigned int check_position;
} belt_t;
// BEGIN TIMINGS

unsigned long long int time_loop(unsigned long long int iters) {
    struct timespec start, end;
    volatile unsigned long long int i;
    clock_gettime(CLOCK_REALTIME, &start);
    clock_gettime(CLOCK_REALTIME, &start);
    for (i = 0; i < iters; i++) {
        
    }
    clock_gettime(CLOCK_REALTIME, &end);
    return (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
}

unsigned long long int num_iter_delay(unsigned long long int target) {
    unsigned long long int min = 1, max = 1, mid;
    while (time_loop(max) < target) {
        max *= 2;
    }
    mid = (min + max) / 2;
    unsigned long long int res = time_loop(mid);
    while (min < max && res != target) {
        if (res < target) {
            min = mid+1;
        } else {
            max = mid-1;
        }
        mid = (min + max) / 2;
        res = time_loop(mid);
    }
    return mid;
}

#ifdef MACOS_SLEEP
void sleep_until(struct timespec *ts, unsigned long long int delay) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    unsigned long long int elapsed = (now.tv_sec - ts->tv_sec) * 1000000000 + (now.tv_nsec - ts->tv_nsec);
    elapsed /= 1000000;
    if (elapsed >= delay) return;
    delay -= elapsed;
    struct timespec new_ts;
    struct timespec remaining;
    new_ts.tv_sec = delay / 1000;
    new_ts.tv_nsec = (delay % 1000)*1000000;
    // print new_ts
    while (nanosleep(&new_ts, &remaining) != 0) {
        new_ts.tv_sec = remaining.tv_sec;
        new_ts.tv_nsec = remaining.tv_nsec;
    }
}
#else
void sleep_until(struct timespec *ts, unsigned long long int delay) {
    struct timespec new_ts;
    unsigned long long int time = ts->tv_nsec + delay * 1000000;
    new_ts.tv_sec = ts->tv_sec + time / 1000000000;
    new_ts.tv_nsec = time % 1000000000;
    while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &new_ts, NULL) == EINTR) {
    }
}

#endif

// END TIMINGS
// BEGIN CAR

#define ENUM_TO_FLAG(part) (1 << part)

const unsigned int FLAGS[NUM_PARTS] = {
    ENUM_TO_FLAG(PART_FRAME),
    ENUM_TO_FLAG(PART_ENGINE),
    ENUM_TO_FLAG(PART_WHEELS),
    ENUM_TO_FLAG(PART_BODY),
    ENUM_TO_FLAG(PART_DOORS),
    ENUM_TO_FLAG(PART_WINDOWS),
    ENUM_TO_FLAG(PART_LIGHTS),
};

const unsigned int REQUIREMENTS[NUM_PARTS] = {
    0,
    FLAGS[PART_FRAME],
    FLAGS[PART_FRAME],
    FLAGS[PART_ENGINE],
    FLAGS[PART_BODY],
    FLAGS[PART_DOORS],
    FLAGS[PART_BODY],
    0,
};

#define GET_REQUIREMENTS(part) REQUIREMENTS[part]

void init_car(car_t *car) {
    if (!car) return;
    car->status = 0;
}

error_t install(car_t *car, part_t part) {
    if (!car) return LINE_STOPPED;
    unsigned int req = GET_REQUIREMENTS(part);
    if (!req || (car->status & req) == req) {
        car->status |= FLAGS[part];
        return OK;
    }
    return INSTALL_REQUIREMENTS;
}

int check_car(car_t *car) {
    if (!car) return INVALID_POINTER;
    return (car->status == (1 << (NUM_PARTS-1)) - 1);
}

// END CAR
// BEGIN STATS

void init_stats(stats_t *stats) {
    if (!stats) return;
    stats->built_cars = 0;
    stats->failed_cars = 0;
    stats->starts = 0;
}

void print_stats(stats_t *stats) {
    if (!stats) return;
    unsigned long long int total = stats->built_cars + stats->failed_cars;
    printf("Total cars: %llu\n", total);
    printf("Success rate: %f (%llu)\n", (float)stats->built_cars / total, stats->built_cars);
    printf("Failure rate: %f (%llu)\n", (float)stats->failed_cars / total, stats->failed_cars);
    printf("Starts: %llu\n", stats->starts);
}

// END STATS
// BEGIN BELT

void init_belt(belt_t *belt) {
    if (!belt) return;
    belt->belt_position = 0;
    belt->check_position = 1;
    for (int i = 0; i < MAX_POSITION*2; i++) {
        belt->arms[i] = PART_EMPTY;
    }
}

error_t install_belt_arm(belt_t *belt, part_t part, side_t side, unsigned int position) {
    if (!belt) return INVALID_POINTER;
    if (position == 0) return INCORRECT_POSITION;
    position--;
    if (position >= MAX_POSITION) return INCORRECT_POSITION;
    unsigned int actual_position = 2*position+side;
    if (belt->arms[actual_position] != PART_EMPTY) return NON_EMPTY_POSITION;
    belt->arms[actual_position] = part;
    if (position+1 >= belt->check_position) belt->check_position = position + 2;
    return OK;
}

void move_belt(belt_t *belt) {
    if (!belt) return;
    belt->belt_position = (belt->belt_position + 1) % (belt->check_position+1);
}

void handle_belt_position(belt_t *belt, car_t *car, stats_t *stats) {
    if (!belt || !car || !stats) return;
    printf("Car in position %d.\n", belt->belt_position);
    if (belt->belt_position == 0) {
        init_car(car);
        printf("New car arriving.\n");
    } else if (belt->belt_position == belt->check_position) {
        printf("Checking car...\n");
        if (check_car(car)) {
            printf_green("Car completed.\n");
            stats->built_cars++;
        } else {
            printf_red("Failed car.\n");
            stats->failed_cars++;
        }
        init_car(car);
    }
}

error_t get_part(belt_t *belt, side_t side, unsigned int position, part_t *part) {
    if (!belt) return INVALID_POINTER;
    error_t res = OK;
    if (belt->belt_position != position) {
        res = INCORRECT_BELT_POSITION;
        goto error;
    }
    if (position == 0) {
        res = INCORRECT_POSITION;
        goto error;
    }
    position--;
    if (position >= MAX_POSITION) {
        res = INCORRECT_POSITION;
        goto error;
    }
    unsigned int actual_position = 2*position+side;
    *part = belt->arms[actual_position];
    if (*part == PART_EMPTY) {
        res = INCORRECT_POSITION;
    }
error:
    return res;
}

// END BELT
// BEGIN ASSEMBLY LINE

struct assembly_line {
    car_t car;
    belt_t belt;
    stats_t stats;
    _Atomic int running;
    unsigned long long int ms_delay;
    sem_t block_sem;
    pthread_mutex_t safe_mutex;
};

error_t init_assembly_line(assembly_line_t *line) {
    srand(time(NULL));
    struct assembly_line *inner = calloc(1, sizeof(struct assembly_line));
    if (inner == NULL) return MALLOC_ERROR;
    init_car(&inner->car);
    init_belt(&inner->belt);
    init_stats(&inner->stats);
    inner->ms_delay = num_iter_delay(1000000);
    inner->running = 0;
    if (sem_init(&inner->block_sem, 0, 0) != 0) {
        goto sem_error;
    }
    if (pthread_mutex_init(&inner->safe_mutex, NULL) != 0) {
        goto mutex_error;
    }
    *line = inner;
    return OK;
mutex_error:
    sem_destroy(&inner->block_sem);
sem_error:
    free(inner);
    return SEM_ERROR;
}

error_t free_assembly_line(assembly_line_t *line) {
    if (line == NULL) return OK;
    struct assembly_line *inner = *line;
    if (inner == NULL) return OK;
    int res = sem_destroy(&inner->block_sem);
    if (res != 0) {
        return SEM_ERROR;
    }
    res = pthread_mutex_destroy(&inner->safe_mutex);
    if (res != 0) {
        return SEM_ERROR;
    }
    free(inner);
    *line = NULL;
    return OK;
}

error_t setup_arm(assembly_line_t line, part_t part, side_t side, unsigned int position) {
    if (line->running) return LINE_STARTED;
    return install_belt_arm(&line->belt, part, side, position);
}

error_t run_assembly(assembly_line_t line) {
    if (line->running) return LINE_STARTED;
    int values;
    sem_getvalue(&line->block_sem, &values);
    for (int i = 0; i < values; i++) {
        sem_wait(&line->block_sem);
    }
    sem_post(&line->block_sem);
    line->running = 1;
    printf_green("Assembly line started.\n");
    struct timespec ts;
    error_t res = OK;
    printf("Last position (end of assembly): %d\n", line->belt.check_position);
    line->belt.belt_position = line->belt.check_position;
    line->stats.starts++;
    while (line->running) {
        if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
            res = TIME_ERROR;
            goto time_error;
        }
        sem_wait(&line->block_sem);
        pthread_mutex_lock(&line->safe_mutex);
        if (line->running) {
            move_belt(&line->belt);
        }
        sem_post(&line->block_sem);
        if (!line->running) {
            pthread_mutex_unlock(&line->safe_mutex);
            goto time_error;
        }
        handle_belt_position(&line->belt, &line->car, &line->stats);
        pthread_mutex_unlock(&line->safe_mutex);
        sleep_until(&ts, BELT_PERIOD);
    }
time_error:
    printf_green("Assembly line stopped.\n");
car_error:
    return res;
}

error_t trigger_arm(assembly_line_t line, side_t side, unsigned int position) {
    printf("Installing in position %d.\n", line->belt.belt_position);
    if (!line->running) return LINE_STOPPED;
    sem_wait(&line->block_sem);
    pthread_mutex_lock(&line->safe_mutex);
    part_t part;
    error_t res = get_part(&line->belt, side, position, &part);
    if (res != OK) {
        goto bad_pos;
    }
    int delay = rand() % (MAX_DELAY - MIN_DELAY + 1) + MIN_DELAY;
    for (volatile unsigned long long int i = 0; i < line->ms_delay*delay; i++) {
        
    }
    res = install(&line->car, part);
bad_pos:
    pthread_mutex_unlock(&line->safe_mutex);
    int block = (rand() % ONE_IN_BLOCK_CHANCE) == 0;
    if (!block && line->running) {
        sem_post(&line->block_sem);
    }
    return res;
}

error_t shutdown_assembly(assembly_line_t line) {
    if (!line->running) return LINE_STOPPED;
    pthread_mutex_lock(&line->safe_mutex);
    printf("Shutting down assembly line.\n");
    int delay = rand() % (MAX_DELAY - MIN_DELAY + 1) + MIN_DELAY;
    for (volatile unsigned long long int i = 0; i < line->ms_delay*delay; i++) {
        
    }
    line->running = 0;
    init_car(&line->car);
    line->belt.belt_position = 0;
    for (int i = 0; i <= MAX_POSITION*2; i++) {
        sem_post(&line->block_sem);
    }
    pthread_mutex_unlock(&line->safe_mutex);
    printf_green("Assembly line shut down.\n");
    return OK;
}

void print_assembly_stats(assembly_line_t line) {
    pthread_mutex_lock(&line->safe_mutex);
    print_stats(&line->stats);
    pthread_mutex_unlock(&line->safe_mutex);
}

// END ASSEMBLY LINE
