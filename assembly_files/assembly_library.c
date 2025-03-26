#include "assembly_library.h"
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

// Signal
void handle_signal(int signum, void (*handler)(int)) {
    struct sigaction act;
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(signum, &act, NULL);
}

// Watchdog
timer_t watchdog() {
    struct sigevent se;
    se.sigev_notify = SIGEV_SIGNAL;
    se.sigev_signo = SIGALRM;
    timer_t timer_id;
    timer_create(CLOCK_REALTIME, &se, &timer_id);
    return timer_id;
}

void pet_watchdog(timer_t timer_id, int duration) {
    struct itimerspec ts;
    ts.it_value.tv_sec = duration / 1000;
    ts.it_value.tv_nsec = (duration % 1000) * 1000000;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;
    timer_settime(timer_id, 0, &ts, NULL);
}

timer_t watchdog_function(void (*fonction) (union sigval)) {
    struct sigevent se;
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_notify_function = fonction;
    se.sigev_value.sival_int = 0;
    se.sigev_notify_attributes = NULL;
    timer_t timer_id;
    timer_create (CLOCK_REALTIME, &se, &timer_id) ;
    return timer_id;
}

// Delay
unsigned long long int measure_loop(unsigned long long int iter, int *r) {
    int local;
    if (r == NULL) {
        r = &local;
    }
    unsigned long long int nanotime = 0;
    struct timespec begin = {0}, end = {0};
    clock_gettime(CLOCK_REALTIME, &begin);
    for (unsigned long long int i = 0; i < iter; ++i) {
        *r ^= i;
    }
    clock_gettime(CLOCK_REALTIME, &end);
    nanotime = (end.tv_sec - begin.tv_sec) * 1000000000 + end.tv_nsec - begin.tv_nsec;
    return nanotime;
}

unsigned long long int compute_iterations(unsigned long long int delay) {
    unsigned long long int lower = 1;
    unsigned long long int upper = 1;
    unsigned long long int nanotime = 0;
    
    while (nanotime < 2 * delay) {
        upper *= 2;
        nanotime = measure_loop(upper, NULL);
    }
    
    unsigned long long int middle = (lower + upper) / 2;
    nanotime = measure_loop(middle, NULL);
    
    while ((lower < upper) && (nanotime > delay || delay > nanotime)) {
        if (nanotime > delay) {
            upper = middle - 1;
        } else if (delay > nanotime) {
            lower = middle + 1;
        }
        middle = (lower + upper) / 2;
        nanotime = measure_loop(middle, NULL);
    }
    return middle;
}

void add_to_time(struct timespec *t, unsigned int delay, const struct timespec *start) {
    if (start != NULL) {
        t->tv_sec = start->tv_sec;
        t->tv_nsec = start->tv_nsec;
    }
    unsigned long int nsec = t->tv_nsec + delay * 1000000;
    t->tv_nsec = nsec % 1000000000;
    t->tv_sec = t->tv_sec + nsec / 1000000000;
}

void delay_until(const struct timespec *start, unsigned int delay) {
    struct timespec end;
    add_to_time(&end, delay, start);
    while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &end, NULL) == EINTR) {}
}