#pragma once

#include <signal.h>
#include <time.h>

// Signal
void handle_signal(int signum, void (*handler)(int));

// Watchdog
timer_t watchdog();
timer_t watchdog_function (void (*fonction) (union sigval));
void pet_watchdog(timer_t timer_id, int duration);