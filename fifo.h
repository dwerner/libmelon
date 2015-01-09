#ifndef DNA_FIFO_H
#define DNA_FIFO_H

#include <pthread.h>

typedef struct node_def {
  struct node_def *next;
  void * data;
} node_t;

typedef struct {
  long size;
  node_t *first;
  node_t *current;
  pthread_cond_t *wait;
  pthread_mutex_t *mutex;
} fifo_t;

node_t *node_create(void *data);
void node_destroy(node_t *node);
void * fifo_pop( fifo_t *fifo );
void fifo_empty( fifo_t *fifo );
int fifo_is_empty( fifo_t *fifo );
void fifo_destroy( fifo_t *fifo );
long fifo_count( fifo_t *fifo );
void fifo_push( fifo_t * fifo, void * item );
fifo_t *fifo_create();

#endif
