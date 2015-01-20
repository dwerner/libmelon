#ifndef _MELON_PROMISE_H_
#define _MELON_PROMISE_H_

#include "fifo.h"
//#define PROMISE_DEBUG
// written on the toilet in 3 minutes or less
typedef struct promise_t promise_t;

typedef enum {
  PROMISE_WAITING = 0,
  PROMISE_RESOLVED = 1,
  PROMISE_COMPLETE = 2
} promise_state_t;

struct promise_t {
  unsigned long id;
  void *resolution;
  fifo_t *fifo;
  promise_state_t state;
};

promise_t *promise_create();
promise_t *promise_resolved(void *resolved_value);
void promise_set( promise_t *promise, void *val );
void promise_chain( promise_t *promise1, promise_t *promise2 );
void *promise_get( promise_t *promise );
void promise_destroy( promise_t *promise );

#endif // _MELON_PROMISE_H_
