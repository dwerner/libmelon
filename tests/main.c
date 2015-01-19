#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <check.h>

#include "melon.h"

static fifo_t *fifo = NULL;

typedef struct {
  int id;
  int value;
} int_message_t;

static int thread_counter = 0;

void fifo_check();

void sleep_for(long seconds);

void test_empty_thread_pool();

void test_empty_fifo();

#define ELEMS 1000
//#define VALUE_LOG
//#define SLEEPAFTER

void *fifo_fill( void *nothing ) {
  int cap = ++thread_counter;
  int i = 0;
  for (i = 0; i < ELEMS; i++) {
    int_message_t *val = (int_message_t*) malloc( sizeof(int_message_t) );
    val->id = cap;
    val->value = i;
    fifo_push( fifo, val );

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

void test_empty_fifo() {
  printf( "<-------------------- test_empty_fifo ---------------------\n");
  int j = 0;
  for (j = 0; j < 10; j++) {
    fifo_t *fifo = fifo_create(" <test fifo>", 0);
    fifo_destroy(fifo);
  }
}

void test_fifo() {
  printf( "<-------------------- test_fifo ---------------------\n");
  int j = 0;
  for (j = 0; j < 10; j++) {
    fifo_t *fifo = fifo_create("<test fifo>", 0);

    fifo_destroy(fifo);
  }
}

void test_empty_thread_pool() {
  printf( "<-------------------- test_empty_thread_pool ---------------------\n");
  int i = 0;
  for (i = 0; i < 10 ; i++ ) {
    printf(">> --- cycle %i --- <<\n", i+1);
    // global here, for all threads to share
    fifo = fifo_create("values", 0);
    printf("starting thread pool\n");
    thread_pool_t *pool = thread_pool_create("main pool", 1);
    printf("destroying thread pool\n");
    thread_pool_destroy(pool);
    fifo_check();
    fifo_destroy(fifo);
  }
}

void test_busy_thread_pool() {
  printf( "<-------------------- test_busy_thread_pool  ---------------------\n");
  fifo = fifo_create("<(busy_thread_pool) value fifo>", 0);
  thread_pool_t *pool = thread_pool_create("<busy thread pool>", 8); // should auto-determine thread count maybe?
  printf("adding %i tasks to the queue...\n", ELEMS);
  int i = 0;
  for( i = 0; i < ELEMS; i++ ) {
    thread_pool_enqueue(pool, &fifo_fill, NULL);
  }
  printf("waiting for threads to complete on their own...\n");
  thread_pool_join_all( pool );
  printf("destroying thread pool...\n");
  thread_pool_destroy( pool );

  fifo_check();
  printf("destroying global value fifo.\n");
  fifo_destroy( fifo );
}

void test_few_tasks_thread_pool() {
  printf( "<-------------------- test_few_tasks_thread_pool  ---------------------\n");
  fifo = fifo_create("<(busy_thread_pool) value fifo>", 0);
  thread_pool_t *pool = thread_pool_create("<few tasks thread pool>", 8); // should auto-determine thread count maybe?
  printf("adding %i tasks to the queue...\n", ELEMS);
  int i = 0;
  for( i = 0; i < 1; i++ ) {
    thread_pool_enqueue(pool, &fifo_fill, NULL);
  }
  printf("waiting for threads to complete on their own...\n");
  thread_pool_join_all( pool );
  printf("destroying thread pool...\n");
  thread_pool_destroy( pool );

  fifo_check();
  printf("destroying global value fifo.\n");
  fifo_destroy( fifo );
}


// consumer side of actor
typedef enum {
  PING = 0,
  PONG = 1,
  DONE = 2
} message_type_t;

// Our user-defined receive method
// Must return: NULL or a promise_t -> a chain of promises or a resolved promise with a value.
// An immediately resolved promise can be created with promise_create_resolved()
promise_t *actor_ping_receive( const actor_t *this, const message_t *msg ) {
  switch( msg->type ) {
    case PING: {
      printf("%s-> PING ->%s %lu\n", this->name, msg->from->name, msg->id);
      message_t *response = actor_message_create(this, NULL, (msg->id < 100 ? PONG : DONE) );
      response->id = msg->id + 1;

      // actor_send returns a promise, so it will chain to the original one
      // When the original promise is resolved ( such as with promise_get() )
      // The entire chain will be resolved, up to the final value.
      return actor_send( msg->from, response );
    };
    case PONG: {
      printf("%s-> PONG ->%s %lu\n", this->name, msg->from->name, msg->id);
      message_t *response = actor_message_create(this, NULL, (msg->id < 100 ? PING : DONE) );
      response->id = msg->id + 1;
      return actor_send( msg->from, response );
    };
    case DONE: {
      // In this case we are done and want to resolve the result of some work.
      // We use promise_create_resolved(); to encapsulate that value as a resolved promise.
      printf("%s-> DONE ->%s %lu\n", this->name, msg->from->name, msg->id);
      return promise_create_resolved( (void*) actor_message_create(this, NULL, DONE) );
    };
    // If this actor did not understand the message it was sent, it will return NULL,
    // and the original promise will be resolved as NULL.
    default: break;
  };
  return NULL;
}

void test_actor_system() {
  printf( "<-------------------- test_actor_system  ---------------------\n");
  actor_system_t *actor_system = actor_system_create("stuff");

  // create actors and add them to the system
  actor_t *actor1 = actor_create(&actor_ping_receive, "actor1");
  actor_t *actor2 = actor_create(&actor_ping_receive, "actor2");
  actor_system_add( actor_system, actor1 );
  actor_system_add( actor_system, actor2 );

  // create a message and send it to the actor, capturing the promise from actor_send
  message_t *message = actor_message_create( actor2, NULL, PING );
  message->id = 1;
  promise_t *promise = actor_send( actor1, message);

  // Starts execution of the actor system - enqueueing receive evaluation on an internal thread pool
  actor_system_run( actor_system );

  // block this thread until we can resolve the promise, in this case a chain of promises
  void *val = promise_get( promise );
  if (val) {
    // it happens to be a message created by the actor and returned
    message_t *response = (message_t*)val;
    printf("resolved promise: %s\n", (response->type == DONE ? "DONE" : "invalid!") );
  }

  // stop actor system, internal thread pool, task list, and clean up
  actor_system_destroy( actor_system );
}

int main(int argc, char *argv[]) {
  test_empty_fifo();
  test_fifo();
  test_empty_thread_pool();
  test_busy_thread_pool();
  test_few_tasks_thread_pool();
  test_actor_system();
  return 0;
}




