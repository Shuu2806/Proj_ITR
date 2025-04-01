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
#ifdef MACOS_SLEEP
void delay_until(struct timespec *ts, unsigned long long int delay) {
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
void delay_until(struct timespec *ts, unsigned long long int delay) {
    struct timespec new_ts;
    unsigned long long int time = ts->tv_nsec + delay * 1000000;
    new_ts.tv_sec = ts->tv_sec + time / 1000000000;
    new_ts.tv_nsec = time % 1000000000;
    while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &new_ts, NULL) == EINTR) {
    }
}

#endif