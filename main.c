#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "fifo.h"
#include "logging.h"
#include "thread_pool.h"

static fifo_t *fifo = NULL;

void fifo_stats() {
  printf("Fifo size : %lu\n", fifo_count(fifo) );
}

typedef struct {
  int id;
  int value;
} message_t;

static int thread_counter = 0;

#define ELEMS 10000

void *fifo_fill( void *nothing ) {
  printf("adding %i value elements to the fifo\n", ELEMS);
  int cap = ++thread_counter;
  int i = 0;
  for (i = 0; i < ELEMS; i++) {
    message_t *val = (message_t*) malloc( sizeof(message_t) );
    val->id = cap;
    val->value = i;
    fifo_push( fifo, val );
    printf("<< added %i to value fifo\n", i);
  }
  return NULL;
}


int main(int argc, char *argv[]) {

  fifo = fifo_create("values");

  printf("starting thread pool\n");
  thread_pool_t *pool = thread_pool_create("main pool", 5);
  thread_pool_enqueue( pool, &fifo_fill, NULL );
  thread_pool_enqueue( pool, &fifo_fill, NULL );
  thread_pool_enqueue( pool, &fifo_fill, NULL );

  struct timespec interval = {
      .tv_sec = (time_t) 1,
      .tv_nsec = 0
  };
  nanosleep(&interval, NULL);
  printf("destroying thread pool super early\n");
  thread_pool_destroy( pool );

  printf("emptying result fifo\n");
  // empty the fifo and see what we have in it
  int ctr=0;

  while(!fifo_is_empty(fifo) )  {
    void *val = fifo_pop( fifo );
    message_t *msg = (message_t*) val;
    free( msg );
    ctr ++;
  }

  printf("Counted %i elements popped from value queue\n", ctr);
  fifo_destroy( fifo );

  return 0;
}


