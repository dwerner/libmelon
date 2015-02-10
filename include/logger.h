#ifndef _MELON_LOGGING_H_
#define _MELON_LOGGING_H_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

enum log_level_t {
  NONE = 0,
  ERROR = 1,
  INFO = 2,
  WARN = 3,
  DEBUG = 4
};

typedef enum log_level_t log_level_t;
#define GLOBAL_LOG_LEVEL WARN 

void dna_log( log_level_t level, const char *fmt, ... );

#endif // _MELON_LOGGING_H_
