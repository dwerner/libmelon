#pragma once

#include <stdio.h>

static int dbg_counter = 0;
void log_count() {
  printf("dbg counter %i\n", ++dbg_counter);
}

/* // Dave -> pointers here?
const char *format(const char* str, int arg1) {
  char *newStr = (char*) malloc(sizeof(str)+1);
  snprintf(newStr, sizeof(newStr), str, arg1);
  return (const char *)newStr;
}
*/

/*
void log(const char * msg, ...) {
  printf("test ", va_args);
}
*/

