#include "fifo.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "threads.h"

node_t *node_create(void *data) {
  node_t *node = malloc( sizeof(node_t) );
  node->next = NULL;
  node->data = data;
  return node;
}

void node_destroy(node_t *node) {
  if (node) { free(node); }
}


fifo_t *fifo_create() {
  fifo_t *fifo = (fifo_t *) malloc( sizeof(fifo_t) );
  fifo->mutex = (pthread_mutex_t*) malloc( sizeof( pthread_mutex_t ) );
  fifo->wait = (pthread_cond_t*) malloc( sizeof( pthread_cond_t ) );
  dna_mutex_init( fifo->mutex );
  dna_cond_init( fifo->wait );
  fifo->size = 0;
  fifo->first = NULL;
  fifo->current = NULL;
  return fifo;
}

// Push a new item into the fifo
void fifo_push_internal( fifo_t *fifo, node_t *node ) {
  dna_mutex_lock( fifo->mutex );
  if (fifo->current) {
    fifo->current->next = node;
  }
  fifo->current = node;
  if ( !fifo->first ) {
    fifo->first = node;
  }
  fifo->size ++;
  dna_cond_signal( fifo->wait );
  dna_mutex_unlock( fifo->mutex );
}

// Pop the first item in the fifo
node_t *fifo_pop_internal( fifo_t *fifo ) {
  node_t *node = NULL;
  dna_mutex_lock( fifo->mutex );
  if (fifo->size == 0) {
    // unlock and then wait to be signalled
    dna_cond_wait( fifo->wait, fifo->mutex );
  }
  if (fifo->first) {
    node = fifo->first;
    fifo->first = node->next;
    fifo->size--;
  }
  dna_mutex_unlock( fifo->mutex );
  return node;
}

void fifo_push( fifo_t * fifo, void * item ) {
  node_t *node = node_create( item );
  fifo_push_internal( fifo, node );
}

void * fifo_pop( fifo_t *fifo ) {
  node_t *node = fifo_pop_internal( fifo );
  void *data = node->data;
  node_destroy( node );
  return data;
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

void fifo_empty( fifo_t *fifo ) {
  dna_mutex_lock( fifo->mutex );
  while( fifo->first ) {
    printf("size: %lu\n", fifo_count( fifo ) );
    node_t *node = fifo_pop_internal( fifo );
    if(node) {
      node_destroy( node );
    }
  }
  dna_mutex_unlock( fifo->mutex );
}

int fifo_is_empty( fifo_t *fifo ) {
  return fifo->size == 0;
}

void fifo_destroy( fifo_t *fifo ) {
  if (fifo) {
    // empty the fifo
    fifo_empty( fifo );
    dna_cond_destroy( fifo->wait );
    dna_mutex_destroy( fifo->mutex );
    free( fifo->wait );
    free( fifo->mutex );
    free( fifo );
  }
}

