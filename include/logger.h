#ifndef _MELON_LOGGING_H_
#define _MELON_LOGGING_H_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

enum log_level_t {
  INFO = 0,
  WARN = 1,
  ERROR = 2,
  DEBUG = 3
};


typedef enum log_level_t log_level_t;

static log_level_t log_level = INFO;

void dna_log( log_level_t level, const char *fmt, ... );



#endif // _MELON_LOGGING_H_
