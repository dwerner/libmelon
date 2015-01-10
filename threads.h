#ifndef DNA_THREADS_H
#define DNA_THREADS_H

#include <pthread.h>

void dna_mutex_lock( pthread_mutex_t *mutex );
void dna_mutex_unlock( pthread_mutex_t *mutex );
void dna_cond_init( pthread_cond_t *cond );
void dna_cond_wait( pthread_cond_t *cond, pthread_mutex_t *mutex );
void dna_cond_signal( pthread_cond_t *cond );
void dna_cond_broadcast( pthread_cond_t *cond );
void dna_cond_destroy( pthread_cond_t *cond );
void dna_mutex_init ( pthread_mutex_t *mutex );
void dna_mutex_destroy ( pthread_mutex_t *mutex );

#endif
