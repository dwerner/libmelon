#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fifo.h"
#include "promise.h"
#include "message.h"
#include "actor.h"
#include "actor_system.h"
#include "logger.h"

/* We MIGHT create a message, or recycle an old one. See actor_system_message_get()/.._put() */
message_t *actor_message_create( actor_t *actor, void *data, int type ) {
  if (actor->actor_system) {
    return actor_system_message_get( actor->actor_system, data, type, actor );
  }
  return NULL;
}

/* perhaps a better name is due - it doesn't destroy, but recycles it */
void actor_message_destroy( actor_t *actor, message_t *message ) {
  message->data = NULL;
  if (message->promise) {
    promise_destroy( message->promise );
  }
  message->promise = NULL;
  message->id = 0;
  message->from = NULL;
  message->type = 0;
  actor_system_message_put( actor->actor_system, message );
}

actor_t *actor_create( receive_func_p receive, const char *name) {
  actor_t *actor = (actor_t*) malloc( sizeof(actor_t) );
  actor->receive = receive;
  actor->name = name;
  actor->mailbox = fifo_create(name, 0);
  actor->pid = 0;
  actor->state = ACTOR_DORMANT;
  actor->livestate = ACTOR_IDLE;
  actor->actor_system = NULL;
  dna_log(DEBUG, "Created actor %s", actor->name);
  return actor;
}

void actor_destroy(actor_t *actor) {
  dna_log(DEBUG, "destroying actor %s", actor->name);
  fifo_destroy( actor->mailbox );
  free( actor );
}

/**
 * Represents a single scheduled receive task for this actor, queued in a thread_pool.
 * Scheduling, at this point, just means queued to run.
 *
 * Notes:
 *  - We pop a single message from the actor's mailbox, and call receive,
 *    capturing the promise resulting from it.
 *  - If the promise derived from the receive call does not contain a realized value,
 *    it's another promise. We make sure that the promise derived will be added to the
 *    existing chain. (Stinky people on the bus make my day.)
 *  - An actor can be 'killed' with actor_kill(), so we need to watch for that state,
 *    and stop the actor from processing more messages. Killing an actor also stops it
 *    from being scheduled in the thread_pool.
 *  - Trouble: 
 *    When code sends an actor a message, a promise is created to represent the eventual
 *    value that will be generated. This value might be one of three things:
 *      a. NULL : simply represent the completion of the work triggered by the message.
 *      b. 'resolved' promise : a promise with .state == PROMISE_RESOLVED
 *      c. 'chained' promise : a promise with .state == PROMISE_CHAINED
 *        - This last option is special: It is a series of 'chained' promises (currently 
 *          implemented as nested fifos) and always ends in a 'resolved' promise. The 
 *          purpose of this type of promise is to allow sequential chaining;
 *          i.e. much work to be completed that will eventually return a promise. 
 *
 *          Perhaps this is better implemented as a single promise that the user is 
 *          forced to pass along with their messages. As a chain of nested fifos, this grows
 *          to a pretty heavy structure in our memory space.
 *
 * Implementation notes:
 *  - A memory optimization: we recycle the messages used and place them in a pool.
 *
 */
void *actor_receive_task_internal(void *arg) {
  actor_t *actor = (actor_t*) arg;
  /* will block the thread, so we should only 
   * execute when there's a message waiting */
  if (!fifo_is_empty(actor->mailbox) && actor->state != ACTOR_DEAD) {

    actor->livestate = ACTOR_AWAKE;
    message_t *msg = (message_t*) fifo_pop( actor->mailbox );
    promise_t *result = actor->receive( actor, msg );
    if (actor->state == ACTOR_DEAD) {
      actor_system_message_put( actor->actor_system, msg );
      // possibly want to destroy the promise here - the actor is now dead
      return result;
    }

    if ( !result ) {
      /* when receive returns NULL, a choice has been made by the user to
       * not use promises. They return NULL because it's more meaningful
       * than a forcing them to return a value, and placing that in a promise.
       * We still resolve this as a value to the caller, as NULL will simply
       * represent completion of the task behind the message this actor received. */
      promise_set( msg->promise, NULL );
    } else {
      if (result->state == PROMISE_RESOLVED) {
        /* If we got a resolved promise we resolve the promise and unblock the caller. */
        promise_set( msg->promise, result->resolution );
      } else if (result->state == PROMISE_WAITING) {
        /* If a promise is still pending, it can only be chained.
           If we got a chained promise, we should chain internally for
           resolution to succeed on the entire chain. */
        promise_chain( msg->promise, result );
      }
    }
    actor_system_message_put( actor->actor_system, msg );
    actor->livestate = ACTOR_IDLE;
  }
  /* If this actor isn't kaput, schedule another receive */
  if (actor->state == ACTOR_ALIVE) {
    thread_pool_enqueue(
        actor->actor_system->thread_pool, 
        &actor_receive_task_internal, 
        actor
    );
  }
  return NULL;
}

/*
 * Actors are scheduled via a thread_queue, this kicks off scheduling.
 */
void actor_spawn( actor_t *actor ) {
  dna_log(DEBUG, "Spawning actor %s.", actor->name);
  if (actor->state == ACTOR_DORMANT) {
    actor->state = ACTOR_ALIVE;

    thread_pool_enqueue(
        actor->actor_system->thread_pool,
        &actor_receive_task_internal,
        actor
    );
  }
}

/**
 * Kill this actor immediately. Prevent this actor from being
 * scheduled to run again, and stop receiving messages.
 * 
 * Side effects:
 *  - If this is the only actor in the actor_system_t it resides in, 
 *  - Any remaining messages in the actor's mailbox will be placed in the
 *    message pool. This presents a problem because the caller might not
 *    know how to clean up data they have sent in their messsages, so a
 *    cleanup hook is provided in the function ptr *cleanup.
 */
void actor_kill( actor_t *actor, void(*cleanup)(void*) ) {
  dna_log(DEBUG, "Killing actor %s.", actor->name);
  actor_system_t *actor_system = actor->actor_system;
  if (actor->state != ACTOR_DEAD) {
    actor->state = ACTOR_DEAD;
    actor_system_remove( actor->actor_system, actor );
    if ( cleanup ) {
      fifo_each(actor->mailbox, cleanup);
    }
    /* Drain any remaining messages to the pool... */
    actor_system_recycle_messages( actor->actor_system, actor->mailbox );
  }
  /* If the last actor has been killed, stop the actor system */
  if ( fifo_is_empty( actor_system->actors ) ) {
    dna_log(DEBUG, "Actor system no longer has any actors within it, stopping...");
    actor_system_stop( actor_system );
  }
}

/** (Move to header)
 * actor_send( actor, message ) ->
 * 
 * Places the message in the actor's mailbox. The next time that the actor is
 * scheduled to run, it will pull one message from it's mailbox, and process it
 * in the user-defined 'receive' function.
 */
promise_t *actor_send( actor_t *actor, message_t *message ) {
  message->promise = promise_create();
  message->promise->id = message->id;
  fifo_push( actor->mailbox, message );
  return message->promise;
}

