#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

#include "fifo.h"
#include "threads.h"

// TODO:
// | Reusability |
// - separate concurrency primitives from fifo_t into new concurrent_fifo_t
//
// | Optimizations |
// - implement an optional size limit for fifo_t
//   - add pthread_cond_t to fifo_t -> wait_room
//   - push would then block if there is no room, and pop would signal that
//
// - implement a pre-allocated region of memory to start with for node_t instances
//   - add recycling of node_t, rather than malloc + free each time.

node_t *node_create( void *data ) {
  node_t *node = (node_t*) malloc( sizeof(node_t) );
  node->next = NULL;
  node->data = data;
  return node;
}

/*
node_t *node_pool_pop(fifo_t *fifo); // could just be another fifo_t
void node_pool_push(fifo_t *fifo, node_t *node );
*/

void node_destroy( node_t *node ) {
  if (node) { free( node ); }
}


#define NODE_PREALLOCATED_DEFAULT 50

fifo_t *fifo_create( const char *name /*, long max_size */ ) {
  fifo_t *fifo = (fifo_t *) malloc( sizeof(fifo_t) );
  if (name) fifo->name = name;
  fifo->mutex = (pthread_mutex_t*) malloc( sizeof( pthread_mutex_t ) );
  fifo->wait_pop = (pthread_cond_t*) malloc( sizeof( pthread_cond_t ) );

  /* // IN PROGRESS - implementation of size limitation and node caching
  long preallocated_nodes = max_size ? max_size : NODE_PREALLOCATED_DEFAULT;
  node_t *node_list = (node_t*) malloc( sizeof(node_t) * preallocated_nodes );
  long l = 0;
  for ( l = 0; l < sizeof(node_t) * preallocated_nodes; l += sizeof(node_t) ) {

  }

  fifo->node_list = node_list;

  if (max_size) {
    fifo->wait_push = (pthread_cond_t*) malloc( sizeof( pthread_cond_t ) );
    dna_cond_init( fifo->wait_push );
  }
  */
  dna_mutex_init( fifo->mutex );
  dna_cond_init( fifo->wait_pop );
  fifo->size = 0;
  fifo->first = NULL;
  fifo->current = NULL;
  return fifo;
}

// Push a new item into the fifo
void fifo_push_internal( fifo_t *fifo, node_t *node ) {
  dna_mutex_lock( fifo->mutex );
  assert( node != NULL );
  if ( fifo_is_empty( fifo ) ) {
    fifo->first = node;
    fifo->current = fifo->first;
  }
  else if ( fifo->current ) {
    fifo->current->next = node;
    fifo->current = node;
  }
  fifo->size ++;
  dna_cond_signal( fifo->wait_pop );
  dna_mutex_unlock( fifo->mutex );
}

// Pop the first item in the fifo
node_t *fifo_pop_internal( fifo_t *fifo ) {
  node_t *node = NULL;
  dna_mutex_lock( fifo->mutex );
  int cond_tries = 0;
  while ( fifo_is_empty(fifo) ) {
    cond_tries ++;
    printf("[%i, %lu] fifo %s still empty, waiting for signal\n",
        cond_tries, fifo->size, fifo->name );
    // unlock and then wait to be signalled - this is a cancellation point
    dna_cond_wait( fifo->wait_pop, fifo->mutex );
    if ( fifo_is_empty(fifo) ) {
      printf("[<%i>] predicate is still null! Spurious wakeup?\n", cond_tries);
      continue;
    }
    printf("fifo is empty (%s) spinning...\n", fifo->name);
  }
  assert( !fifo_is_empty(fifo) );
  node = fifo->first;
  fifo->first = node->next; // even if it's NULL (end of the list)
  fifo->size --;
  dna_mutex_unlock( fifo->mutex );
  return node;
}

int fifo_is_empty( fifo_t *fifo ) {
  return fifo->first == NULL;
}

void fifo_push( fifo_t * fifo, void *item ) {
  node_t *node = node_create( item );
  fifo_push_internal( fifo, node );
}

void * fifo_pop( fifo_t *fifo ) {
  node_t *node = fifo_pop_internal( fifo );
  if (node) {
    void *data = node->data;
    node_destroy(node);
    return data;
  } else {
    return NULL;
  }
}

// locks
long fifo_count( fifo_t *fifo ) {
  long count = 0;
  dna_mutex_lock( fifo->mutex );
  node_t *head = fifo->first;
  while( head ) {
    count ++;
    head = head->next;
  }
  dna_mutex_unlock( fifo->mutex );
  return count;
}

// This needs to clean up the data for the
// consumer, or allow a callback to do so
void fifo_empty( fifo_t *fifo ) {
  dna_mutex_lock( fifo->mutex );
  while( !fifo_is_empty( fifo ) ) {
    node_t *node = fifo->first;
    if (node) {
      node_t *next = node->next;
      node_destroy(node);
      if (next) {
        fifo->first = next; // even if it's NULL (end of the list)
      } else {
        break;
      }
    }
  }
  fifo->first = NULL;
  fifo->current = NULL;
  fifo->size = 0;
  dna_mutex_unlock( fifo->mutex );
}

int fifo_any( fifo_t *fifo, int(*predicate)(const void*) ) {
  dna_mutex_lock(fifo->mutex);
  if ( predicate ) {
    node_t *head = fifo->first;
    while( head ) {
      int result = predicate((const void *) head->data);
      if (result) break;
      head = head->next;
    }
  }
  dna_mutex_unlock(fifo->mutex);
  return 0;
}

void fifo_each(fifo_t *fifo, void(*func)(void *) ) {
  dna_mutex_lock(fifo->mutex);
  if ( func ) {
    node_t *head = fifo->first;
    while( head ) {
      func( head->data );
      head = head->next;
    }
  }
  dna_mutex_unlock(fifo->mutex);
}


// Clean up after and free a fifo
void fifo_destroy( fifo_t *fifo ) {
  if (fifo) {
    printf("destroying fifo %s...\n", fifo->name);
    fifo_empty( fifo );
    dna_mutex_destroy( fifo->mutex );
    dna_cond_destroy( fifo->wait_pop );
    free( fifo->wait_pop );
    free( fifo->mutex );
    free( fifo );
  }
}

