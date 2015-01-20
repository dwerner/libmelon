#include <stdlib.h>
#include <stdio.h>

#include "fifo.h"
#include "promise.h"

typedef enum {
  VALUE = 0,
  PROMISE_CHAIN = 1
} value_type_t;

typedef struct {
  void* value;
  value_type_t type;
} value_t;

#ifdef PROMISE_DEBUG
#define promise_print(X) printf(X"\n")
#define log_id(X) \
printf("promise id: %lu\n", X->id);
#else
#define promise_print()
#define log_id() {}
#endif

promise_t * promise_create() {
  promise_t *promise = (promise_t*) malloc( sizeof(promise_t) );
  promise->fifo = fifo_create("promise", 1);
  promise->state = PROMISE_WAITING;
  return promise;
}

promise_t *promise_resolved(void *resolved_value) {
  promise_t *promise = (promise_t*) malloc( sizeof(promise_t) );
  promise->fifo = NULL;
  promise->state = PROMISE_RESOLVED;
  promise->resolution = resolved_value;
  return promise;
}

void promise_chain(promise_t *promise1, promise_t *promise2 ) {
  value_t *value = (value_t*) malloc( sizeof(value_t) );
  value->type = PROMISE_CHAIN;
  value->value = promise2;
  fifo_push(promise1->fifo, value);
}

void promise_set(promise_t *promise, void *val) {
  value_t *value = (value_t*) malloc( sizeof(value_t) );
  value->type = VALUE;
  value->value = val;
  fifo_push( promise->fifo, value );
  promise->state = PROMISE_RESOLVED;
}

void *promise_get(promise_t *promise) {
  value_t *value = fifo_pop( promise->fifo );
  while( value->type == PROMISE_CHAIN ) {
    promise_t *next = (promise_t*) value->value;
    free( value );
    value = (value_t*) fifo_pop( next->fifo );
  }
  void *val = value->value;
  free(value);
  promise->state = PROMISE_COMPLETE;
  return val;
}

void promise_destroy(promise_t *promise) {
  fifo_destroy( promise->fifo );
  free(promise);
}
