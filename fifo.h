#ifndef DNA_FIFO_H
#define DNA_FIFO_H

#include <pthread.h>

typedef struct node_def {
  long id;
  struct node_def *next;
  void * data;
} node_t;

typedef struct named_node_def {
  struct named_node_def *next;
  const char *name;
  void *data;
} named_node_t;

typedef struct {
  const char *name;
  long size;
  node_t *first;
  node_t *current;
  pthread_cond_t *wait;
  pthread_mutex_t *mutex;
} fifo_t;

node_t *node_create( void *data, long id );
void node_destroy( node_t *node );
void * fifo_pop( fifo_t *fifo );
void fifo_empty( fifo_t *fifo );
int fifo_is_empty( fifo_t *fifo );
void fifo_destroy( fifo_t *fifo );
long fifo_count( fifo_t *fifo );
void fifo_push( fifo_t * fifo, void * item, long id );
fifo_t *fifo_create( const char *name );

#endif
