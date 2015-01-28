#ifndef _MELON_THREADS_H_
#define _MELON_THREADS_H_

#include <pthread.h>

typedef struct dna_thread_context_t dna_thread_context_t;

typedef enum {
  IDLE = 0,
  RUNNING,
  SHOULD_QUIT,
  HAS_QUIT
} runstate_t;

struct dna_thread_context_t {
  long id;
  pthread_mutex_t *mutex; //used to lock access to the runstate
  runstate_t runstate;
  pthread_t *thread;
  int locked;
};

dna_thread_context_t *dna_thread_context_create( long id );
void dna_thread_context_execute( dna_thread_context_t *context, void* (*func)(void*), void *args );
void dna_thread_context_exit(dna_thread_context_t *context);
int dna_thread_context_should_exit(dna_thread_context_t *context);
void dna_thread_context_destroy(dna_thread_context_t *context);
void dna_thread_context_join( dna_thread_context_t * ctx );


void dna_mutex_lock( pthread_mutex_t *mutex );
void dna_mutex_unlock( pthread_mutex_t *mutex );
void dna_cond_init( pthread_cond_t *cond );
void dna_cond_wait( pthread_cond_t *cond, pthread_mutex_t *mutex );
void dna_cond_timedwait( pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime  );
void dna_cond_signal( pthread_cond_t *cond );
void dna_cond_broadcast( pthread_cond_t *cond );
void dna_cond_destroy( pthread_cond_t *cond );
void dna_mutex_init ( pthread_mutex_t *mutex );
void dna_mutex_destroy ( pthread_mutex_t *mutex );
void dna_thread_cancel( pthread_t *thread );
void dna_thread_detach( pthread_t *thread );

#endif // _MELON_THREADS_H_
