#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "fifo.h"
#include "logging.h"
#include "thread_pool.h"

static fifo_t *fifo = NULL;

typedef struct {
  int id;
  int value;
} message_t;

static int thread_counter = 0;

void fifo_check();

void sleep_for(long seconds);

#define ELEMS 10
#define VALUE_LOG
//#define SLEEPAFTER

void *fifo_fill( void *nothing ) {
  int cap = ++thread_counter;
  int i = 0;
  for (i = 0; i < ELEMS; i++) {
    message_t *val = (message_t*) malloc( sizeof(message_t) );
    val->id = cap;
    val->value = i;
    fifo_push( fifo, val, 0 );
#ifdef VALUE_LOG
    printf("<< added %i to value fifo\n", i);
#endif
  }
  return NULL;
}


// empty the fifo and see what we have in it
void fifo_check() {
  printf("emptying result fifo...\n");
  int ctr = 0;
  while( !fifo_is_empty(fifo) )  {
    void *val = fifo_pop( fifo );
    if (val) {
      message_t *msg = (message_t *) val;
      free(msg);
      ctr++;
    }
  }
  printf("Counted %i elements popped from value queue\n", ctr);
}

void sleep_for(long seconds) {
  printf("Sleeping for %lu", seconds);
  struct timespec interval = {
      .tv_sec = (time_t) seconds
  };
  nanosleep(&interval, NULL);
}

int main(int argc, char *argv[]) {
  int j = 0;

  for (j = 0; j < 10; j++) {
    fifo_t *fifo = fifo_create("<test fifo>");
    fifo_destroy(fifo);
  }

  for (j = 0; j < 100 ; j++ ) {
    printf("--- cycle %i\n", j);
    fifo = fifo_create("values");
    printf("starting thread pool\n");
    thread_pool_t *pool = thread_pool_create("main pool", 1);
#ifdef SLEEPAFTER
    long seconds = 2;
    sleep_for(seconds);
#endif
    printf("destroying thread pool\n");
    thread_pool_destroy(pool);
    fifo_check();
    fifo_destroy(fifo);
  }
  return 0;
}



