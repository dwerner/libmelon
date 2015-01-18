#include "fifo.h"

#include "promise.h"

promise_t * promise_create() {
  return (promise_t*) fifo_create("promise", 1);
}

void promise_set(promise_t *promise, void *val) {
  fifo_t *fifo = (fifo_t*) promise;
  fifo_push( fifo, val );
}

void *promise_get(promise_t *promise) {
  fifo_t *fifo = (fifo_t*) promise;
  return fifo_pop( fifo );
}

void promise_destroy(promise_t *promise) {
  fifo_destroy( (fifo_t*) promise );
}
