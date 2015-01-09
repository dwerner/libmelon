#include <string.h>
#include <pthread.h>
#include <stdlib.h>

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

void *fifo_fill( void *nothing ) {
  int cap = ++thread_counter;
  int i = 0;
  for (i = 0; i < 1000; i++) {
    message_t *val = (message_t*) malloc( sizeof(message_t) );
    val->id = cap;
    val->value = i;
    fifo_push( fifo, val );
  }
  return NULL;
}

int main(int argc, char *argv[]) {

  fifo = fifo_create();


  printf("starting static threads\n");
  pthread_t thread1;
  pthread_t thread2;
  pthread_create(&thread1, NULL, &fifo_fill, NULL);
  pthread_create(&thread2, NULL, &fifo_fill, NULL);
  printf("joining static threads\n");
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  printf("starting thread pool\n");
  thread_pool_t *pool = thread_pool_create(2);
  thread_pool_enqueue( pool, &fifo_fill, NULL );
  thread_pool_enqueue( pool, &fifo_fill, NULL );
  thread_pool_enqueue( pool, &fifo_fill, NULL );
  thread_pool_join_all( pool );
  printf("destroying thread pool\n");
  thread_pool_destroy( pool );

  printf("emptying result fifo\n");
  // empty the fifo and see what we have in it
  int ctr1=0;
  int ctr2=0;
  while(!fifo_is_empty(fifo) )  {
    void *val = fifo_pop( fifo );
    message_t *msg = (message_t*) val;
    if (msg->id == 1) ctr1 ++;
    else ctr2 ++;
    free( msg );
  }

  printf("Counted %i elements popped from thread 1\n", ctr1);
  printf("Counted %i elements popped from thread 2\n", ctr2);
  fifo_destroy( fifo );

  return 0;
}


