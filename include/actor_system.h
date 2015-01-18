
#ifndef ACTOR_SYSTEM_H
#define ACTOR_SYSTEM_H

#include "thread_pool.h"
#include "actor.h"

typedef struct {
  const char *name;
  fifo_t *actors;
  thread_pool_t *thread_pool;
} actor_system_t;

actor_system_t *actor_system_create(const char *name);
void actor_system_add( actor_system_t *actor_system, actor_t *actor );
void actor_system_run( actor_system_t *actor_system );
void actor_system_destroy( actor_system_t *actor_system );

#endif
