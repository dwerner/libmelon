#include <stdio.h>
#include <pthread.h>

#include "threads.h"


void dna_mutex_lock( pthread_mutex_t *mutex ) {
  pthread_mutex_lock( mutex );
}

void dna_mutex_unlock( pthread_mutex_t *mutex ) {
  pthread_mutex_unlock( mutex );
}

void dna_cond_init( pthread_cond_t *cond ) {
  pthread_cond_init( cond, NULL );
}

void dna_cond_wait( pthread_cond_t *cond, pthread_mutex_t *mutex ) {
  pthread_cond_wait( cond, mutex );
}

void dna_cond_signal( pthread_cond_t *cond ) {
  pthread_cond_signal( cond );
}

void dna_cond_broadcast( pthread_cond_t *cond ) {
  pthread_cond_broadcast( cond );
}

void dna_cond_destroy( pthread_cond_t *cond ) {
  int code = 0;
  while( (code = pthread_cond_destroy( cond )) ) {
    if (code == 16 /*EBUSY*/) {
      printf("Couldn't destory cond variable - was EBUSY. Trying again...\n");
    } else {
      printf("Couldn't destory cond variable (code %i)\n", code);
    }
  }
}

void dna_mutex_init( pthread_mutex_t *mutex ) {
  pthread_mutex_init( mutex, NULL );
}

void dna_mutex_destroy( pthread_mutex_t *mutex ) {
  if (mutex) {
    pthread_mutex_destroy( mutex );
  }
}
