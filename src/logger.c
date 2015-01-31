#include <stdlib.h>
#include <stdarg.h>

#include "logger.h"

#define LABEL_LENGTH 6
#define INFO_LABEL "INFO :"
#define WARN_LABEL "WARN :"
#define ERROR_LABEL "ERROR:"
#define DEBUG_LABEL "DEBUG:"

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
