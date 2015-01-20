#pragma once

#include "actor.h"
#include "fifo.h"
#include "thread_pool.h"

/***
* TODO:
* - allow manual purging of the message pool (memory utilization/fragmentation)
* - block allocation at startup; initialize with a pool of a certain size
*/

typedef struct actor_t actor_t;
typedef struct message_t message_t;
typedef struct actor_system_t actor_system_t;

struct actor_system_t {
  const char *name;
  fifo_t *actors;

  // messages, due to being prolific, are recycled and
  // cleaned up when the actor system is destroyed
  fifo_t *message_pool;
  thread_pool_t *thread_pool;
};

actor_system_t *actor_system_create(const char *name);
void actor_system_add( actor_system_t *actor_system, actor_t *actor );
void actor_system_run( actor_system_t *actor_system );
void actor_system_destroy( actor_system_t *actor_system );

message_t *actor_system_message_get( actor_system_t *actor_system, void *data, int type, const actor_t *from );
void actor_system_recycle_messages( actor_system_t *actor_system, fifo_t *message_fifo );
void actor_system_message_put( actor_system_t *actor_system, message_t *message );
