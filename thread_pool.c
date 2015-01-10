#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "fifo.h"
#include "thread_pool.h"

typedef struct {
  void* (*func)(void*);
  void *arg;
} task_t;

task_t *task_create( void*(*func)(void*), void *arg ) {
  task_t *task = (task_t*) malloc( sizeof( task_t ) );
  task->func = func;
  task->arg = arg;
  return task;
}

void *task_execute( task_t *task ) {
  // Explicitly mark this as a cancellation point.
  pthread_testcancel();
  void* (*func)(void*) = task->func;
  void *arg = task->arg;
  if ( arg ) {
    return func( arg );
  } else {
    return func( NULL );
  }
}

void task_destroy( task_t *task ) {
  free( task );
}

/**
* Until cancellation, pull a task_t out of the task queue and run it on our thread
*/
void *execute_task_thread_internal( void *task_list ) {

  // threads within the pool must be cancellable.
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

  fifo_t *tasks = (fifo_t*) task_list;
  task_t *task = NULL;
  while ( (task = (task_t*) fifo_pop( tasks ) ) ) {
    pthread_testcancel();
    printf(">> executing task \n");
    //void *result =
    // TODO: handle callback?
    task_execute( task );
    task_destroy( task );
  }
  return NULL;
}

/***
* Create a thread pool, including a fifo of tasks.
* Starts consuming threads immediately.
*/
thread_pool_t *thread_pool_create( const char *name, int thread_count ) {
  assert( thread_count > 0 );
  thread_pool_t *pool = (thread_pool_t*) malloc( sizeof( thread_pool_t ) );
  pool->thread_queue = fifo_create(name);
  pool->tasks = fifo_create(name);
  int i = 0;
  for (i = 0; i < thread_count; i++) {
    pthread_t *thread = (pthread_t*) malloc( sizeof( pthread_t ) );
    pthread_create( thread, NULL, &execute_task_thread_internal, pool->tasks );
    fifo_push( pool->thread_queue, (void*)thread );
  }
  return pool;
}

void thread_pool_join_all( thread_pool_t *pool ) {
  while (!fifo_is_empty( pool->thread_queue )) {
    printf("-- joining thread...\n");
    pthread_t *thread = (pthread_t *) fifo_pop(pool->thread_queue);
    pthread_join(*thread, NULL);
  }
}

void thread_pool_destroy( thread_pool_t *pool ) {
  printf("thread_pool_destroy\n");
  if ( pool ) {
    while ( !fifo_is_empty( pool->thread_queue ) ) {
      pthread_t *thread = (pthread_t*) fifo_pop( pool->thread_queue );
      printf("cancelling thread...\n");
      int i = 0;
      while( (i = pthread_cancel(*thread)) ){
        printf("thread cancellation failed: %i\n", i);
      }
      // unblock all threads waiting on the fifo so they can be cancelled
      dna_cond_broadcast( pool->thread_queue->wait );
      free( thread );
    }
    while ( !fifo_is_empty( pool->tasks ) ) {
      printf("destroying tasks\n");
      task_t *task = (task_t*) fifo_pop( pool->tasks );
      task_destroy( task );
    }
    fifo_destroy( pool->tasks );
    fifo_destroy( pool->thread_queue );
    free( pool );
  }
}

void thread_pool_enqueue_task( thread_pool_t *pool, task_t *task ) {
  fifo_push( pool->tasks, task );
}

void thread_pool_enqueue( thread_pool_t *pool, void*(*func)(void*), void *arg) {
  printf("<< enqueuing task\n");
  task_t *task = task_create( func, arg );
  thread_pool_enqueue_task( pool, task );
}
