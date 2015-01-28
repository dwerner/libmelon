#ifndef _MELON_FIFO_H_
#define _MELON_FIFO_H_

#include <pthread.h>

typedef struct node_t node_t;
typedef struct fifo_t fifo_t;

struct node_t {
  node_t *next;
  void * data;
};

struct fifo_t {
  const char *name;
  long size;
  node_t *first;
  node_t *current;
  pthread_cond_t *wait_pop;
  pthread_cond_t *wait_push;
  pthread_mutex_t *mutex;
  node_t *node_cache;
};

void*fifo_pop( fifo_t *fifo );
//void*fifo_peek( fifo_t *fifo );
//fifo_t *fifo_filter( fifo_t *fifo )
//fifo_t *fifo_extract( fifo_t *fifo, int(*predicate)(const void*) );
int  fifo_any( fifo_t *fifo, int(*predicate)(const void*) );
void fifo_each(fifo_t *fifo, void(*func)(void *) );
void fifo_empty( fifo_t *fifo );
int  fifo_is_empty( fifo_t *fifo );
void fifo_destroy( fifo_t *fifo );
long fifo_count( fifo_t *fifo );
void fifo_push( fifo_t * fifo, void * item );
fifo_t *fifo_create( const char *name, long max_size );

#endif // _MELON_FIFO_H_
