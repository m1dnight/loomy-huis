#pragma once

#include <time.h>

time_t get_current_time();

void print_current_time_with_message(const char *message);

void print_time_and_message(time_t time, const char *message);

void print_current_time();

void initialize_sntp(void);

void stop_sntp();
