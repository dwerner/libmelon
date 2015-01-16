#ifndef DNA_FIFO_H
#define DNA_FIFO_H

#include <pthread.h>


typedef struct {
  const char *name;
  long size;
  node_t *first;
  node_t *current;
  pthread_cond_t *wait;
  pthread_mutex_t *mutex;
} fifo_t;

void*fifo_pop( fifo_t *fifo );
int  fifo_any( fifo_t *fifo, int(*predicate)(const void*) );
void fifo_each(fifo_t *fifo, void(*func)(void *) );
void fifo_empty( fifo_t *fifo );
int  fifo_is_empty( fifo_t *fifo );
void fifo_destroy( fifo_t *fifo );
long fifo_count( fifo_t *fifo );
void fifo_push( fifo_t * fifo, void * item );
fifo_t *fifo_create( const char *name );

#endif
