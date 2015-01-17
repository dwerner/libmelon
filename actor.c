#include "fifo.h"
#include "promise.h"
#include "actor.h"

message_t *message_create(void *data, unsigned int type, void *from) {
  message_t *message = (message_t*) malloc( sizeof(message_t) );
  message->data = data;
  message->type = type;
  message->promise = NULL;
  message->from = from;
  message->id = 0;  // id should increment...
  return message;
}

void message_destroy(message_t *message) {
  promise_destroy( message->promise );
  free( message );
}

actor_t *actor_create(void*(*receive)(const actor_t*, const message_t*), const char *name) {
  actor_t *actor = (actor_t*) malloc( sizeof(actor_t) );
  actor->receive = receive;
  actor->pool = NULL;
  actor->name = name;
  actor->mailbox = fifo_create(name, 0);
  actor->pid = 0;
  actor->state = ACTOR_DORMANT;
  actor->livestate = ACTOR_IDLE;
  return actor;
}

void *actor_receive_task_internal(void *arg) {
  actor_t *actor = (actor_t*) arg;
  // will block the thread, so we should only execute when there's a message waiting
  if (!fifo_is_empty(actor->mailbox)) {
    actor->livestate = ACTOR_AWAKE;
    message_t *msg = (message_t*) fifo_pop( actor->mailbox );
    void *result = actor->receive( actor, msg );
    promise_set( msg->promise, result );
    actor->livestate = ACTOR_IDLE;
  }
  // If this actor isn't kaput, schedule another receive
  if (actor->state == ACTOR_ALIVE) {
    thread_pool_enqueue(actor->pool, &actor_receive_task_internal, arg);
  }
  return NULL;
}

void actor_spawn(actor_t *actor) {
  // now what does it mean to spawn an actor? start it processing messages
  if (actor->state == ACTOR_DORMANT) {
    actor->state = ACTOR_ALIVE;
    thread_pool_enqueue(actor->pool, &actor_receive_task_internal, actor);
  }
}

void destroy_message_internal( void *arg ) {
  message_t *message = (message_t*) arg;
  message_destroy(message);
}

void actor_kill(actor_t *actor) {
  actor->state = ACTOR_DEAD;
  fifo_each( actor->mailbox, &destroy_message_internal );
}

promise_t *actor_send(actor_t *actor, message_t *message) {
  message->promise = promise_create();
  fifo_push( actor->mailbox, message );
  return message->promise;
}

void actor_destroy(actor_t *actor) {
  actor_kill( actor );
  fifo_empty( actor->mailbox );
  fifo_destroy( actor->mailbox );
}
