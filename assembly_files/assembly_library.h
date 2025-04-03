#pragma once

#include <signal.h>
#include <time.h>

// Signal
void handle_signal(int signum, void (*handler)(int));

// Watchdog
timer_t watchdog();
timer_t watchdog_function (void (*fonction) (union sigval));
void pet_watchdog(timer_t timer_id, int duration);

// delay
void add_to_time(struct timespec *t, unsigned long long int delay, const struct timespec *start);
void delay_until(const struct timespec *start, unsigned long long int delay);