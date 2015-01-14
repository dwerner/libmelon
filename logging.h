#ifndef DNA_UTILS_H
#define DNA_UTILS_H
#include <stdio.h>

static int dbg_counter = 0;
void log_count() {
  printf("dbg counter %i\n", ++dbg_counter);
}

/*
void log(const char * msg, ...) {
  printf("test ", va_args);
}
*/

#endif