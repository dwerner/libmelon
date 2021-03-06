#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "fifo.h"
#include "thread_pool.h"
#include "threads.h"
#include "logger.h"

typedef struct {
  void* (*func)(void*);
  void *arg;
} task_t;

typedef struct {
  fifo_t *task_list;
  dna_thread_context_t *thread_context;
} execution_args_t;

task_t *task_create( void*(*func)(void*), void *arg ) {
  task_t *task = (task_t*) malloc( sizeof( task_t ) );
  task->func = func;
  task->arg = arg;
  return task;
}

execution_args_t *execution_args_create(thread_pool_t *pool, dna_thread_context_t *context) {
  execution_args_t *args = (execution_args_t*) malloc(sizeof(execution_args_t));
  args->thread_context = context;
  args->task_list = pool->tasks;
  return args;
}

/* Actually execute work from the thread pool. */
void *task_execute( task_t *task ) {
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
* Until pthread_exit, pull a task_t out of the task queue and run it on our thread
*/
void *execute_task_thread_internal( void *args ) {
  execution_args_t *yargs = (execution_args_t*) args;
  fifo_t *tasks = yargs->task_list;
  dna_thread_context_t *context = yargs->thread_context;
  free( yargs );
  dna_log(DEBUG, "started execution of thread %lu", context->id);
  task_t *task = NULL;
  while ( !dna_thread_context_should_exit(context) &&
          (task = (task_t*) fifo_pop( tasks ) ) ) {

    if ( task->func ) { // thread_pool_destroy sends tasks which have NULL members in, don't bother executing it
      task_execute(task);
    }
    task_destroy( task );
    if (dna_thread_context_should_exit(context)) {
      break;
    }
  }
  dna_log(DEBUG, "Execution of thread %lu has finished.", context->id );
  return NULL;
}

/***
* Create a thread pool, including a fifo of tasks.
* Starts consuming threads immediately.
*/
thread_pool_t *thread_pool_create( const char *name, int thread_count ) {
  assert( thread_count > 0 );
  thread_pool_t *pool = (thread_pool_t*) malloc( sizeof( thread_pool_t ) );
  pool->mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  dna_mutex_init(pool->mutex);
  pool->wait = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
  dna_cond_init(pool->wait);
  pool->name = name;
  pool->tasks = fifo_create("(tasks)", 50 );
  pool->thread_queue = fifo_create("(threads)", thread_count );
  int i = 0;
  for ( i = 0; i < thread_count; i++ ) {
    dna_thread_context_t *context = dna_thread_context_create(i+1);
    execution_args_t *args = execution_args_create(pool, context);
    dna_thread_context_execute(context, &execute_task_thread_internal, args);
    fifo_push( pool->thread_queue, context );
  }
  return pool;
}

// used to mark all threads so they will quit
void kill_thread(void *arg) {
  dna_thread_context_t *context = (dna_thread_context_t *) arg;
  dna_thread_context_exit(context);
}

void delete_task( void *arg ) {
  task_t *task = (task_t*)arg;
  task_destroy(task);
}

void thread_pool_exit_all( thread_pool_t *pool ) {
  dna_mutex_lock( pool->mutex );
  fifo_each( pool->tasks, &delete_task );
  fifo_empty(pool->tasks);
  fifo_each(
      pool->thread_queue,
      &kill_thread
  );
  /* push new "work" into the queue to unblock threads waiting on the list */
  int x = 0;
  for ( x = 0; x < fifo_count( pool->thread_queue ); x++) {
    /* We guard and don't execute NULL function pointers
       This merely meets the needs of the fifo for unblocking. */
    thread_pool_enqueue( pool, NULL, NULL );
  }
  dna_cond_signal( pool->wait );
  dna_mutex_unlock( pool->mutex );
}

int thread_context_is_running(const void * arg ) {
  const dna_thread_context_t *ctx = (const dna_thread_context_t*)arg;
  return ctx->runstate == RUNNING;
}

/***
* The thread pool has the property that all the threads will keep running until signalled to quit.
* Each thread will call pthread_exit() once it has completed execution of it's current 'task_t'.
*/
void thread_pool_join_all( thread_pool_t *pool ) {
  dna_log(DEBUG, "Joining thread pool:");
  while ( !fifo_is_empty(pool->thread_queue) ) {
    dna_mutex_lock( pool->mutex );
    int threadsAreStillRunning = 0;
    while ( (threadsAreStillRunning = fifo_any( pool->thread_queue, &thread_context_is_running)) ) {
      dna_log(DEBUG, "Some threads are still running...%i", threadsAreStillRunning);
      dna_cond_wait( pool->wait, pool->mutex );
    }
    dna_thread_context_t *context = (dna_thread_context_t*) fifo_pop( pool->thread_queue );
    if (context->runstate == RUNNING) {
      dna_log(WARN, "context is still running, placing back in the queue.");
      fifo_push( pool->thread_queue, context );
    }
    else {
      // if the context is != RUNNING, it's either SHOULD_QUIT or HAS_QUIT
      // so we have to rely on the null work we pushed earlier to clear
      // any blocks
      dna_thread_context_join( context );
      dna_thread_context_destroy(context);
    }
    dna_mutex_unlock( pool->mutex );
  }
  dna_log(DEBUG, "Thread pool joined.");
}

void thread_pool_destroy( thread_pool_t *pool ) {
  dna_log(DEBUG, "Inside thread_pool_destroy()");
  if ( pool ) {
    dna_log(DEBUG, "Telling threads to exit...");
    thread_pool_join_all(pool);
    dna_log(DEBUG, "Destroying execution context fifo...");
    fifo_destroy( pool->thread_queue );
    pool->thread_queue = NULL;
    dna_log(DEBUG, "Destroying tasks in fifo...");
    while ( !fifo_is_empty( pool->tasks ) ) {
      task_t *task = (task_t*) fifo_pop( pool->tasks );
      task_destroy( task );
    }
    fifo_destroy( pool->tasks );
    pool->tasks = NULL;
    dna_log(DEBUG, "Freeing thread context pool \"%s\".", pool->name);
    dna_mutex_destroy( pool->mutex );
    dna_cond_destroy( pool->wait );
    free(pool->mutex);
    free(pool->wait);
    free( pool );
  }
}

void thread_pool_enqueue_task( thread_pool_t *pool, task_t *task ) {
  fifo_push( pool->tasks, task );
}

void thread_pool_enqueue( thread_pool_t *pool, void*(*func)(void*), void *arg) {
  task_t *task = task_create( func, arg );
  thread_pool_enqueue_task( pool, task );
}
