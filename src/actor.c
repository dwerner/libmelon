#include <stdlib.h>

#include "fifo.h"
#include "promise.h"
#include "message.h"
#include "actor.h"
#include "actor_system.h"


// We MIGHT create a message, or recycle an old one.
message_t *actor_message_create( const actor_t *actor, void *data, int type ) {
  if (actor->actor_system) {
    return actor_system_message_get( actor->actor_system, data, type, actor );
  }
  return NULL;
}

// perhaps a better name is due - it doesn't destroy, but recycles it
void actor_message_destroy( const actor_t *actor, message_t *message ) {
  message->data = NULL;
  message->promise = NULL;
  message->id = 0;
  message->from = NULL;
  message->type = 0;
  actor_system_message_put( actor->actor_system, message );
}

actor_t *actor_create( receive_func_p receive, const char *name) {
  actor_t *actor = (actor_t*) malloc( sizeof(actor_t) );
  actor->receive = receive;
  actor->pool = NULL;
  actor->name = name;
  actor->mailbox = fifo_create(name, 0);
  actor->pid = 0;
  actor->state = ACTOR_DORMANT;
  actor->livestate = ACTOR_IDLE;
  actor->actor_system = NULL;
  return actor;
}

void *actor_receive_task_internal(void *arg) {
  actor_t *actor = (actor_t*) arg;
  // will block the thread, so we should only execute when there's a message waiting
  if (!fifo_is_empty(actor->mailbox)) {
    actor->livestate = ACTOR_AWAKE;

    message_t *msg = (message_t*) fifo_pop( actor->mailbox );
    promise_t *result = actor->receive( actor, msg );

    if ( !result ) {
      // If receive returns NULL, we most likely didn't handle a message.
      // Either way, there's nothing to work with then.
      promise_set( msg->promise, NULL );
    } else {
      if (result->state == RESOLVED) {
        // If we got a resolved promise (no fifo, just a void*)
        // we resolve the promise and unblock the caller.
        promise_set( msg->promise, result->resolution );
      } else if (result->state == CHAINED) {
        // If we got a chained promise, we should chain internally for
        // resolution to succeed on the entire chain.
        promise_chain( msg->promise, result );
      }
    }
    // recycle the message
    actor_system_message_put( actor->actor_system, msg );
    actor->livestate = ACTOR_IDLE;
  }
  // If this actor isn't kaput, schedule another receive
  if (actor->state == ACTOR_ALIVE) {
    thread_pool_enqueue(actor->pool, &actor_receive_task_internal, actor);
  }
  return NULL;
}

void actor_spawn( actor_t *actor ) {
  // now what does it mean to spawn an actor? start it processing messages
  if (actor->state == ACTOR_DORMANT) {
    actor->state = ACTOR_ALIVE;
    thread_pool_enqueue( actor->pool, &actor_receive_task_internal, actor );
  }
}

void actor_kill( actor_t *actor, void(*cleanup)(void*) ) {
  actor->state = ACTOR_DEAD;
  // run a cleanup callback on the mailbox if one is provided - don't free messages!
  if(cleanup) {
    fifo_each(actor->mailbox, cleanup);
  }
  // drain any remaining messages to the fifo...
  actor_system_recycle_messages(actor->actor_system, actor->mailbox);
}

promise_t *actor_send(const actor_t *actor, message_t *message) {
  message->promise = promise_create();
  fifo_push( actor->mailbox, message );
  return message->promise;
}

void actor_destroy(actor_t *actor) {
  actor_kill( actor, NULL );
  fifo_empty( actor->mailbox );
  fifo_destroy( actor->mailbox );
}
