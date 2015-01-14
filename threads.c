#include <stdio.h>
#include <pthread.h>

#include "threads.h"

dna_thread_context_t *dna_thread_context_create() {
  dna_thread_context_t *context = (dna_thread_context_t*) malloc(sizeof(dna_thread_context_t));
  context->mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  dna_mutex_init(context->mutex);
  context->runstate = IDLE;
  return context;
}

void dna_thread_context_execute( dna_thread_context_t *context, void* (*func)(void*), void *args ) {
  dna_mutex_lock(context->mutex);
  context->runstate = RUNNING;
  context->thread = (pthread_t*) malloc(sizeof(pthread_t));
  pthread_create(context->thread, NULL, func, args);
  dna_mutex_unlock(context->mutex);
}

void dna_thread_context_exit(dna_thread_context_t *context) {
  dna_mutex_lock(context->mutex);
  context->runstate = SHOULD_QUIT;
  dna_mutex_unlock(context->mutex);
}


int dna_thread_context_should_exit(dna_thread_context_t *context) {
  int should_exit = 0;
  dna_mutex_lock(context->mutex);
  should_exit = context->runstate == SHOULD_QUIT;
  dna_mutex_unlock(context->mutex);
  return should_exit;
}

void dna_thread_context_destroy(dna_thread_context_t *context) {
  if (context) {
//    free(context->thread); // should we ?
    dna_mutex_destroy(context->mutex);
    free(context);
  }
}

void dna_mutex_lock( pthread_mutex_t *mutex ) {
  int code = 0;
  while((code = pthread_mutex_lock( mutex ) )) {
    printf("Unable to lock mutex (%i), trying again...\n", code);
  }
}

void dna_mutex_unlock( pthread_mutex_t *mutex ) {
  int code = 0;
  while((code = pthread_mutex_unlock( mutex ) )) {
    printf("Unable to unlock mutex (%i), trying again...\n", code);
  }
}

void dna_cond_init( pthread_cond_t *cond ) {
  pthread_cond_init( cond, NULL );
}

void dna_cond_wait( pthread_cond_t *cond, pthread_mutex_t *mutex ) {
  int code = 0;
  while ((code = pthread_cond_wait( cond, mutex ) )) {
    printf("cond_wait failed (%i), trying again...\n", code);
  }
  pthread_testcancel();
}

void dna_cond_signal( pthread_cond_t *cond ) {
  int code = 0;
  while ((code = pthread_cond_signal( cond ) )) {
    printf("Couldn't signal (%i), trying again...\n", code);
  }
}

void dna_cond_broadcast( pthread_cond_t *cond ) {
  int code = 0;
  while ((code = pthread_cond_broadcast( cond ))) {
    printf("Couldn't broadcast (%i), trying again...\n", code);
  }
}

void dna_cond_destroy( pthread_cond_t *cond ) {
  int code = 0;
  while( (code = pthread_cond_destroy( cond )) ) {
    if (code == 16 /*EBUSY*/) {
      printf("Couldn't destroy cond variable - was EBUSY. Trying again...\n");
    } else {
      printf("Couldn't destroy cond variable (code %i)\n", code);
    }
  }
}

void dna_mutex_init( pthread_mutex_t *mutex ) {
  pthread_mutexattr_t *attr = (pthread_mutexattr_t*)malloc(sizeof(pthread_mutexattr_t));
  pthread_mutexattr_init(attr);
  pthread_mutexattr_settype(attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init( mutex, attr );
}

void dna_mutex_destroy( pthread_mutex_t *mutex ) {
  if (mutex) {
    int code = 0;
    while ((code = pthread_mutex_destroy( mutex ) )) {
      printf("Unable to destroy mutex (%i), trying again...\n", code);
    }
  }
}

void dna_thread_cancel( pthread_t *thread ) {
  int i = 0;
  while( (i = pthread_cancel(*thread)) ){
    printf("thread cancellation failed: %i, trying again...\n", i);
  }
}
void dna_thread_detach( pthread_t *thread ) {
  int i = 0;
  while( (i = pthread_detach(*thread)) ){
    printf("thread detach failed: %i, trying again...\n", i);
  }
}
