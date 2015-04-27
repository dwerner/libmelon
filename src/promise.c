#include <stdlib.h>
#include <stdio.h>

#include "fifo.h"
#include "promise.h"
#include "logger.h"

typedef enum {
  VALUE = 0,
  PROMISE_CHAIN = 1
} value_type_t;

typedef struct {
  void* value;
  value_type_t type;
} value_t;

promise_t * promise_create() {
  promise_t *promise = (promise_t*) malloc( sizeof(promise_t) );
  promise->fifo = fifo_create("promise", 1);
  promise->state = PROMISE_WAITING;
  return promise;
}

promise_t *promise_resolved( void *resolved_value ) {
  promise_t *promise = (promise_t*) malloc( sizeof(promise_t) );
  promise->fifo = NULL;
  promise->state = PROMISE_RESOLVED;
  promise->resolution = resolved_value;
  return promise;
}

/* Results in a Cons style list of fifos. [1,[2,[3,[4,[5,[value]]]]]]
 Would it be more optimal to simply chain all promises as elements in the same list?
 - TODO: evaluate this idea - expand the promise type and push each promise in order to the list
 - Notwithstanding aliasing issues, perhaps we could point each promise at the same fifo? */
void promise_chain( promise_t *promise1, promise_t *promise2 ) {
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

/* Returns the value from the promise and destroys it.
   Will actually call promise_destroy() + free( promise ); */
void *promise_get( promise_t *promise ) {
  value_t *value = (value_t*) fifo_pop( promise->fifo );
  promise_destroy( promise );
  long i = 1;
  while( value->type == PROMISE_CHAIN ) {
    i++;
    promise_t *next = (promise_t*) value->value;
    free( value );
    value = (value_t*) fifo_pop( next->fifo );
    //promise_destroy( next );
  }
  dna_log(DEBUG, "end of promise chain %lu...", i);
  void *val = value->value;
  free(value);
  return val;
}

void promise_destroy(promise_t *promise) {
  if ( promise ) {
    while ( !fifo_is_empty(promise->fifo) ) {
      value_t *value = (value_t *) fifo_pop(promise->fifo);
      free(value);
    }
    fifo_destroy( promise->fifo );
    promise->fifo = NULL;
    free(promise);
  }
}
