#ifndef _MELON_ACTOR_H_
#define _MELON_ACTOR_H_


#include "promise.h"
#include "thread_pool.h"
#include "message.h"
#include "actor_system.h"

typedef struct actor_t actor_t;

typedef enum {
  ACTOR_DORMANT = 0,
  ACTOR_ALIVE,
  ACTOR_DEAD
} actor_state_t;

typedef enum {
  ACTOR_IDLE = 0,
  ACTOR_AWAKE
} actor_livestate_t;

typedef promise_t*(*receive_func_p)(const actor_t*, const message_t*);

struct actor_t {
  unsigned long pid;
  const char *name;
  receive_func_p receive;
  fifo_t *mailbox;
  actor_state_t state;
  actor_livestate_t livestate;
  // private
  actor_system_t *actor_system;
  thread_pool_t *pool;
};

// These message utils are a facade over actor_system_message_get/put
message_t *actor_message_create(const actor_t *actor, void *data, int type );
void actor_message_destroy(const actor_t *actor, message_t *message );

actor_t *actor_create( receive_func_p receive, const char *name);
void actor_spawn(actor_t *actor);
void actor_kill( actor_t *actor, void(*cleanup)(void*) );
promise_t *actor_send(const actor_t *actor, message_t *message);
void actor_destroy( actor_t *actor );

#endif // _MELON_ACTOR_H_
