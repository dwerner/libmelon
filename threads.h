#ifndef DNA_THREADS_H
#define DNA_THREADS_H

#include <pthread.h>

void dna_mutex_lock( pthread_mutex_t *mutex ) {
  pthread_mutex_lock( mutex );
}

void dna_mutex_unlock( pthread_mutex_t *mutex ) {
  pthread_mutex_unlock( mutex );
}

void dna_cond_init( pthread_cond_t *cond ) {
  pthread_cond_init( cond, NULL);
}

void dna_cond_wait( pthread_cond_t *cond, pthread_mutex_t *mutex ) {
  pthread_cond_wait( cond, mutex);
}

void dna_cond_signal( pthread_cond_t *cond ) {
  pthread_cond_signal( cond );
}

void dna_cond_destroy( pthread_cond_t *cond ) {
  pthread_cond_destroy( cond );
}

void dna_mutex_init ( pthread_mutex_t *mutex ) {
  pthread_mutex_init( mutex, NULL );
}

void dna_mutex_destroy ( pthread_mutex_t *mutex ) {
  if (mutex) {
    pthread_mutex_destroy( mutex );
  }
}

#endif
