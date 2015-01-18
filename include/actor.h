#ifndef ACTOR_H
#define ACTOR_H

#include "promise.h"
#include "actor.h"
#include "thread_pool.h"

typedef enum {
  ACTOR_DORMANT = 0,
  ACTOR_ALIVE,
  ACTOR_DEAD
} actor_state_t;

typedef enum {
  ACTOR_IDLE = 0,
  ACTOR_AWAKE
} actor_livestate_t;

typedef struct {
  int type; // map to user enum
  unsigned long id;
  void *data;
  void *from; // until I get some sleep, forward references are mystifying me with typedef struct members
  promise_t *promise;
} message_t;

typedef struct actor_def {
  unsigned long pid;
  const char *name;
  void*(*receive)(const struct actor_def*, const message_t*);
  thread_pool_t *pool;
  fifo_t *mailbox;
  actor_state_t state;
  actor_livestate_t livestate;
} actor_t;

message_t *message_create(void *data, unsigned int type, void *from);
void message_destroy( message_t *message );

actor_t *actor_create(void*(*receive)(const actor_t*, const message_t*), const char *name);
void actor_spawn(actor_t *actor);
void actor_kill(actor_t *actor);
promise_t *actor_send(actor_t *actor, message_t *message);
void actor_destroy( actor_t *actor );

#endif
