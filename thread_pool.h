#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <assert.h>

#include "fifo.h"

typedef struct {
  void *(*func)(void*);
  void *arg;
} task_t;

task_t *task_create( void*(*func)(void*), void *arg) {
  task_t *task = (task_t*) malloc( sizeof( task_t ) );
  task->func = func;
  task->arg = arg;
  return task;
}

void *task_execute( task_t *task ) {
  return task->func( task->arg );
}

void task_destroy( task_t *task ) {
  free( task );
}

typedef struct {
  fifo_t *tasks;
  fifo_t *thread_queue;
} thread_pool_t;


/**
* Until cancellation, pull a task_t out of the task queue and run it on our thread
*/
void *execute_task_thread_internal( void *task_list ) {
  fifo_t *tasks = (fifo_t*) task_list;
  task_t *task = NULL;
  while ( (task = (task_t*) fifo_pop( tasks ) ) ) {
    printf("executing task\n");
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
thread_pool_t *thread_pool_create( int thread_count ) {
  assert( thread_count > 0 );
  thread_pool_t *pool = (thread_pool_t*) malloc( sizeof( thread_pool_t ) );
  pool->thread_queue = fifo_create();
  pool->tasks = fifo_create();
  int i = 0;
  for (i = 0; i < thread_count; i++) {
    pthread_t *thread = (pthread_t*) malloc( sizeof( pthread_t ) );
    pthread_create( thread, NULL, &execute_task_thread_internal, pool->tasks );
    fifo_push( pool->thread_queue, (void*)thread );
  }
  return pool;
}

void thread_pool_join_all( thread_pool_t *pool ) {
  printf("joining all threads\n");
  while (!fifo_is_empty( pool->thread_queue )) {
    pthread_t *thread = (pthread_t *) fifo_pop(pool->thread_queue);
    pthread_join(*thread, NULL);
    free( thread );
  }
}

void thread_pool_destroy( thread_pool_t *pool ) {
  if ( pool ) {
    while (!fifo_is_empty( pool->thread_queue )) {
      pthread_t *thread = (pthread_t*) fifo_pop( pool->thread_queue );
      pthread_cancel( *thread );
      free( thread );
    }
    while (!fifo_is_empty( pool->tasks )) {
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
  printf("enqueuing task\n");
  task_t *task = task_create( func, arg );
  thread_pool_enqueue_task( pool, task );
}


#endif