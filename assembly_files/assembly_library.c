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

// delay 
void add_to_time(struct timespec *t, unsigned long long int delay, const struct timespec *start) {
    if (start != NULL) {
        t->tv_sec = start->tv_sec;
        t->tv_nsec = start->tv_nsec;
    }
    unsigned long int nsec = t->tv_nsec + delay * 1000000;
    t->tv_nsec = nsec % 1000000000;
    t->tv_sec = t->tv_sec + nsec / 1000000000;
}

void delay_until(const struct timespec *start, unsigned long long int delay) {
    struct timespec end;
    add_to_time(&end, delay, start);
    while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &end, NULL) == EINTR) {}
}