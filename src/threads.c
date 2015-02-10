#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "threads.h"
#include "logger.h"

dna_thread_context_t *dna_thread_context_create( long id ) {
  dna_thread_context_t *context = (dna_thread_context_t*) malloc(sizeof(dna_thread_context_t));
  context->mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  context->runstate = IDLE;
  context->id = id;
  dna_mutex_init( context->mutex );
  return context;
}

void dna_thread_context_execute( dna_thread_context_t *context, void* (*func)(void*), void *args ) {
  dna_mutex_lock(context->mutex);
  context->locked++;
  context->runstate = RUNNING;
  context->thread = (pthread_t*) malloc(sizeof(pthread_t));
  pthread_create(context->thread, NULL, func, args);
  dna_mutex_unlock(context->mutex);
}

void dna_thread_context_exit( dna_thread_context_t *context ) {
  dna_mutex_lock( context->mutex );
  context->runstate = SHOULD_QUIT;
  dna_mutex_unlock( context->mutex );
}

int dna_thread_context_should_exit( dna_thread_context_t *context ) {
  dna_mutex_lock( context->mutex );
  int quit = context->runstate == SHOULD_QUIT;
  dna_mutex_unlock( context->mutex );
  return quit;
}

void dna_thread_context_destroy( dna_thread_context_t *context ) {
  if (context) {
    free( context->thread );
    dna_mutex_destroy(context->mutex);
    free( context->mutex );
    free(context);
  }
}

void dna_mutex_lock( pthread_mutex_t *mutex ) {
  int code = 0;
  while((code = pthread_mutex_lock( mutex ) )) {
    dna_log(ERROR, "Unable to lock mutex (%i), trying again...", code);
  }
}

void dna_mutex_unlock( pthread_mutex_t *mutex ) {
  int code = 0;
  while((code = pthread_mutex_unlock( mutex ) )) {
    dna_log(ERROR, "Unable to unlock mutex (%i), trying again...", code);
  }
}

void dna_cond_init( pthread_cond_t *cond ) {
  pthread_cond_init( cond, NULL );
}

void dna_cond_timedwait( pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime  ) {
  int code = 0;
  while ((code = pthread_cond_timedwait( cond, mutex, abstime) )) {
    dna_log(ERROR, "cond_timedwait failed (%i), trying again...", code);
  }
}

void dna_cond_wait( pthread_cond_t *cond, pthread_mutex_t *mutex ) {
  int code = 0;
  while ((code = pthread_cond_wait( cond, mutex ) )) {
    dna_log(ERROR, "cond_wait failed (%i), trying again...", code);
  }
}

void dna_cond_signal( pthread_cond_t *cond ) {
  int code = 0;
  while ((code = pthread_cond_signal( cond ) )) {
    dna_log(ERROR, "Couldn't signal (%i), trying again...", code);
  }
}

void dna_cond_broadcast( pthread_cond_t *cond ) {
  int code = 0;
  while ((code = pthread_cond_broadcast( cond ))) {
    dna_log(ERROR, "Couldn't broadcast (%i), trying again...", code);
  }
}

void dna_cond_destroy( pthread_cond_t *cond ) {
  int code = 0;
  while( (code = pthread_cond_destroy( cond )) ) {
    if (code == 16 /*EBUSY*/) {
      dna_log(ERROR, "Couldn't destroy cond variable - was EBUSY. Trying again...");
    } else {
      dna_log(ERROR, "Couldn't destroy cond variable (code %i)", code);
    }
  }
}

static pthread_mutexattr_t mutex_attr;

void dna_mutex_init( pthread_mutex_t *mutex ) {
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init( mutex, &mutex_attr );
}

void dna_mutex_destroy( pthread_mutex_t *mutex ) {
  if (mutex) {
    int code = 0;
    while ((code = pthread_mutex_destroy( mutex ) )) {
      dna_log(ERROR, "Unable to destroy mutex (%i), trying again...", code);
    }
  }
}

void dna_thread_cancel( pthread_t *thread ) {
  int i = 0;
  while( (i = pthread_cancel(*thread)) ){
    dna_log(DEBUG, "thread cancellation failed: %i, trying again...", i);
  }
}

void dna_thread_detach( pthread_t *thread ) {
  int i = 0;
  while( (i = pthread_detach(*thread)) ){
    dna_log(DEBUG, "thread detach failed: %i, trying again...", i);
  }
}

void dna_thread_context_join( dna_thread_context_t * ctx ) {
  pthread_join( *ctx->thread, NULL );
}
