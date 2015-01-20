#ifndef _MELON_THREAD_POOL_H_
#define _MELON_THREAD_POOL_H_

#include "fifo.h"
#include "threads.h"

typedef struct thread_pool_t thread_pool_t;

struct thread_pool_t {
  const char *name;
  fifo_t *tasks;
  fifo_t *thread_queue;
};


/***
* Create a thread pool, including a fifo of tasks.
* Starts consuming tasks immediately.
*/
thread_pool_t *thread_pool_create( const char *name, int thread_count );
void thread_pool_join_all( thread_pool_t *pool );
void thread_pool_destroy( thread_pool_t *pool );
void thread_pool_enqueue( thread_pool_t *pool, void*(*func)(void*), void *arg );

#endif // _MELON_THREAD_POOL_H_
