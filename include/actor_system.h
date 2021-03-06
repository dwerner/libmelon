#ifndef _MELON_ACTOR_SYSTEM_H_
#define _MELON_ACTOR_SYSTEM_H_

#include "actor.h"
#include "fifo.h"
#include "thread_pool.h"

/***
* TODO:
* - allow manual purging of the message pool (memory utilization/fragmentation)
* - block allocation at startup; initialize with a pool of a certain size
void actor_system_clear_messages( actor_system_t *actor_system );
*/


typedef struct actor_t actor_t;
typedef struct message_t message_t;
typedef struct actor_system_t actor_system_t;

/*
 - actor_system_t
   Composed of container for the actors to run in, and a thread pool to schedule them on
*/
struct actor_system_t {
  const char *name;
  fifo_t *actors;
  fifo_t *message_pool;
  thread_pool_t *thread_pool;
};

actor_system_t *actor_system_create(const char *name);
void actor_system_add( actor_system_t *actor_system, actor_t *actor );
void actor_system_remove( actor_system_t *actor_system, actor_t *actor );
void actor_system_run( actor_system_t *actor_system );
void actor_system_stop( actor_system_t * actor_system );
void actor_system_destroy( actor_system_t *actor_system );

message_t *actor_system_message_get( actor_system_t *actor_system, void *data, int type, actor_t *from );
void actor_system_recycle_messages( actor_system_t *actor_system, fifo_t *message_fifo );
void actor_system_message_put( actor_system_t *actor_system, message_t *message );

#endif //_MELON_ACTOR_SYSTEM_H_
