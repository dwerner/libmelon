#ifndef PROMISE_H
#define PROMISE_H

#include "fifo.h"

// written on the toilet in 3 minutes or less

typedef fifo_t promise_t;

promise_t *promise_create();
void promise_set( promise_t *promise, void *val );
void *promise_get( promise_t *promise );
void promise_destroy( promise_t *promise );

#endif

