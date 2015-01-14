#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "fifo.h"
#include "thread_pool.h"
#include "threads.h"

typedef struct {
  void* (*func)(void*);
  void *arg;
} task_t;

typedef struct {
  fifo_t *task_list;
  dna_thread_context_t *thread_context;
} execution_args_t;

execution_args_t *execution_args_create(thread_pool_t *pool, dna_thread_context_t *context);

task_t *task_create( void*(*func)(void*), void *arg ) {
  task_t *task = (task_t*) malloc( sizeof( task_t ) );
  task->func = func;
  task->arg = arg;
  return task;
}
// explicity allows null func+args. Null tasks are useful to unblock the queue
void *task_execute( task_t *task ) {
  // Explicitly mark this as a cancellation point.
  void* (*func)(void*) = task->func;
  void *arg = task->arg;
  void *result = NULL;
  if (func) {
    if (arg) {
      result = func(arg);
    } else {
      result = func(NULL);
    }
  }
  return result;
}

void task_destroy( task_t *task ) {
  free( task );
}

/**
* Until cancellation, pull a task_t out of the task queue and run it on our thread
*/
void *execute_task_thread_internal( void *args ) {

  // threads within the pool must be cancellable.
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

  execution_args_t *yargs = (execution_args_t*) args;
  fifo_t *tasks = yargs->task_list;
  dna_thread_context_t *context = yargs->thread_context;

  task_t *task = NULL;
  while ( !dna_thread_context_should_exit(context) &&
          (task = (task_t*) fifo_pop( tasks ) ) ) {

    printf(">> executing task \n");
    //void *result =
    // TODO: handle callback?
    task_execute( task );
    task_destroy( task );
  }

  context->runstate = HAS_QUIT;
  pthread_exit(NULL);
  return NULL;
}

/***
* Create a thread pool, including a fifo of tasks.
* Starts consuming threads immediately.
*/
thread_pool_t *thread_pool_create( const char *name, int thread_count ) {
  assert( thread_count > 0 );
  thread_pool_t *pool = (thread_pool_t*) malloc( sizeof( thread_pool_t ) );
  pool->name = name;
  pool->tasks = fifo_create("(tasks)");
  pool->thread_queue = fifo_create("(threads)");
  int i = 0;
  for ( i = 0; i < thread_count; i++ ) {
    dna_thread_context_t *context = dna_thread_context_create();
    execution_args_t *args = execution_args_create(pool, context);
    dna_thread_context_execute(context, &execute_task_thread_internal, args);
    fifo_push( pool->thread_queue, (void*)context, (long)i ); //keep track of an id for threads
  }
  return pool;
}

execution_args_t *execution_args_create(thread_pool_t *pool, dna_thread_context_t *context) {
  execution_args_t *args = (execution_args_t*) malloc(sizeof(execution_args_t));
  args->thread_context = context;
  args->task_list = pool->tasks;
  return args;
}

void thread_pool_join_all( thread_pool_t *pool ) {
  while (!fifo_is_empty( pool->thread_queue )) {
  }
}

void thread_pool_destroy( thread_pool_t *pool ) {
  printf("thread_pool_destroy\n");
  if ( pool ) {
    printf("cancelling threads...\n");
    dna_thread_context_t *context = NULL;
    while ( !fifo_is_empty( pool->thread_queue )) {
      context = (dna_thread_context_t*) fifo_pop( pool->thread_queue );
      dna_thread_context_exit(context);
      thread_pool_enqueue(pool, NULL, NULL);
      pthread_detach(context->thread);
    }

    printf("destroying execution context fifo...\n");
    fifo_destroy( pool->thread_queue );
    pool->thread_queue = NULL;

    printf("destroying tasks in fifo...\n");
    thread_pool_enqueue(pool, NULL, NULL);

    if ( fifo_is_empty( pool->tasks ) ) {
      thread_pool_enqueue(pool, NULL, NULL);
      task_t *task = (task_t*) fifo_pop( pool->tasks );
      task_destroy( task );
    }

    fifo_destroy( pool->tasks );
    pool->tasks = NULL;

    printf("freeing context pool : (%s).\n", pool->name);
    free( pool );
  }
}

void thread_pool_enqueue_task( thread_pool_t *pool, task_t *task ) {
  // for now, we wont keep track of ids for tasks
  fifo_push( pool->tasks, task, 0 );
}

void thread_pool_enqueue( thread_pool_t *pool, void*(*func)(void*), void *arg) {
  printf("<< enqueuing task\n");
  task_t *task = task_create( func, arg );
  thread_pool_enqueue_task( pool, task );
}
