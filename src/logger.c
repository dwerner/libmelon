#include <stdlib.h>
#include <stdarg.h>

#include "logger.h"

#define LABEL_LENGTH 5+6+5

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"

#define INFO_LABEL KGRN"INFO :"RESET
#define WARN_LABEL KYEL"WARN :"RESET
#define ERROR_LABEL KRED"ERROR:"RESET
#define DEBUG_LABEL KCYN"DEBUG:"RESET

void log_label_internal(const char *label, const char *fmt, char *dest) {
  strncat( dest, label, LABEL_LENGTH);
  strncat( dest, fmt, strlen(fmt) );
  strncat( dest, "\n", 1 );
}

void dna_log( log_level_t level, const char *fmt, ... ) {
  va_list args;
  char *dest = (char*) malloc(strlen(fmt) + 1 + LABEL_LENGTH);
  memset(dest, 0, strlen(fmt)+1+ LABEL_LENGTH);
  va_start(args, fmt);
  if ( level >= log_level ) {
    switch (level) {
      case INFO: {
        log_label_internal(INFO_LABEL, fmt, dest);
        vprintf(dest, args);
        break;
      }
      case WARN: {
        log_label_internal(WARN_LABEL, fmt, dest);
        vprintf(dest, args);
        break;
      }
      case ERROR: {
        log_label_internal(ERROR_LABEL, fmt, dest);
        vprintf(dest, args);
        break;
      }
      case DEBUG: {
        log_label_internal(DEBUG_LABEL, fmt, dest);
        vprintf(dest, args);
        break;
      }
      default:
        break;
    }
  }
  va_end(args);
  free(dest);
}
