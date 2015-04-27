#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

#include "fifo.h"
#include "threads.h"
#include "logger.h"

/* 
 TODO:
 | Reusability |
 - separate concurrency primitives from fifo_t into new concurrent_fifo_t

 | Optimizations |
 - implement an optional size limit for fifo_t
   - add pthread_cond_t to fifo_t -> wait_room
   - push would then block if there is no room, and pop would signal that

 - implement a pre-allocated region of memory to start with for node_t instances
   - add recycling of node_t, rather than malloc + free each time.
*/

node_t *node_create( void *data ) {
  node_t *node = (node_t*) malloc( sizeof(node_t) );
  node->next = NULL;
  node->data = data;
  return node;
}

node_t *node_cache_pop( fifo_t *fifo ) {
  if (fifo->node_cache) {
    node_t *item = fifo->node_cache;
    fifo->node_cache = item->next;
    item->next = NULL;
    item->data = NULL;
    return item;
  }
  return NULL;
}

void node_cache_push( fifo_t *fifo, node_t *node ) {
  if (node) {
    node->data = NULL;
    node->next = NULL;
    node_t *head = NULL;
    while ( (head = fifo->node_cache) ) {
      if (!head->next) {
        head->next = node;
      }
    }
  }
}

void node_destroy( node_t *node ) {
  if (node) { free( node ); }
}

fifo_t *fifo_create( const char *name, long max_size ) {
  fifo_t *fifo = (fifo_t *) malloc( sizeof(fifo_t) );
  if (name) fifo->name = name;
  fifo->mutex = (pthread_mutex_t*) malloc( sizeof( pthread_mutex_t ) );
  fifo->wait_pop = (pthread_cond_t*) malloc( sizeof( pthread_cond_t ) );
  dna_mutex_init( fifo->mutex );
  dna_cond_init( fifo->wait_pop );
  fifo->size = 0;
  fifo->first = NULL;
  fifo->current = NULL;
  return fifo;
}

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

node_t *fifo_pop_internal( fifo_t *fifo ) {
  node_t *node = NULL;
  dna_mutex_lock( fifo->mutex );
  int cond_tries = 0;
  while ( fifo_is_empty(fifo) ) {
    cond_tries ++;
    dna_log(VERBOSE, "[tries: %i, size:%lu] fifo '%s' still empty, waiting for signal.", 
        cond_tries, fifo->size, fifo->name );
    /* unlock and then wait to be signalled - this is a cancellation point
     the challenge is: if we don't break out of this other than when
     there is an item in the queue, we will block for destroy...
     So we should push a NULL task. */
    dna_cond_wait( fifo->wait_pop, fifo->mutex );
    if ( !fifo_is_empty(fifo) ) {
      break;
    } else {
      dna_log(VERBOSE, "[<%i>] predicate is still null! Spurious wakeup?", cond_tries);
    }
  }
  assert( !fifo_is_empty(fifo) );
  node = fifo->first;
  fifo->first = node->next; /* even if it's NULL (end of the list) */
  fifo->size --;
  dna_mutex_unlock( fifo->mutex );
  return node;
}

/* FIXME: leads to a deadlock currently */
int fifo_is_empty( fifo_t *fifo ) {
  int empty = 0;
  dna_mutex_lock( fifo->mutex );
  empty = !fifo || !fifo->first;
  dna_mutex_unlock( fifo->mutex );
  return empty;
}

void fifo_push( fifo_t * fifo, void *item ) {
  node_t *node = node_create( item );
  fifo_push_internal( fifo, node );
}

void *fifo_pop( fifo_t *fifo ) {
  node_t *node = fifo_pop_internal( fifo );
  if (node) {
    void *data = node->data;
    node_destroy(node);
    return data;
  } else {
    return NULL;
  }
}

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

/* TODO: This needs to clean up the data for the
   consumer, or allow a callback to do so */
void fifo_empty( fifo_t *fifo ) {
  dna_mutex_lock( fifo->mutex );
  while( !fifo_is_empty( fifo ) ) {
    node_t *node = fifo->first;
    if (node) {
      node_t *next = node->next;
      node_destroy(node);
      if (next) {
        fifo->first = next;
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
  int result = 0;
  dna_mutex_lock(fifo->mutex);
  if ( predicate ) {
    node_t *head = fifo->first;
    while( head ) {
      result = predicate((const void *) head->data);
      if (result) break;
      head = head->next;
    }
  }
  dna_mutex_unlock(fifo->mutex);
  return result;
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

void fifo_destroy( fifo_t *fifo ) {
  if (fifo) {
    dna_log(DEBUG, "Destroying fifo %s...", fifo->name);
    fifo_empty( fifo );
    dna_cond_destroy( fifo->wait_pop );
    dna_mutex_destroy( fifo->mutex );
    free( fifo->wait_pop );
    free( fifo->mutex );
    free( fifo );
  }
}

