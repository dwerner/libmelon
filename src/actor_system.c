#include <stdlib.h>

#include "message.h"
#include "promise.h"
#include "actor_system.h"
#include "stdio.h"

#define ACTOR_SYSTEM_LOG

actor_system_t *actor_system_create(const char* name){
  actor_system_t *actor_system = (actor_system_t*) malloc( sizeof(actor_system_t) );
  actor_system->name = name;
  actor_system->message_pool = fifo_create("message pool", 0 /* TODO:message pool size!? */);
  actor_system->actors = fifo_create("actors", 0);
  actor_system->thread_pool = thread_pool_create("actor system thread pool", 8 /* CPU detection here? */);
  return actor_system;
}

void actor_system_add(actor_system_t *actor_system, actor_t *actor) {
  actor->actor_system = actor_system;
  fifo_push( actor_system->actors, actor );
}

void actor_system_remove( actor_system_t *actor_system, actor_t *actor ) {
  if ( !fifo_is_empty(actor_system->actors) ) {
    int i = 0;
    long len = fifo_count( actor_system->actors );
    for ( i = 0; i < len; i++ ) {
      actor_t *test = (actor_t*) fifo_pop( actor_system->actors );
      if (test == actor) {
        return;
      } else {
        fifo_push( actor_system->actors, test );
      }
    }
  }
}

void spawn_actor( void *arg ) {
  actor_t *actor = (actor_t*)arg;
  if ( actor && actor->actor_system ) {
    actor_spawn(actor);
  }
}

void actor_system_run(actor_system_t *actor_system) {
  fifo_each( actor_system->actors, &spawn_actor );
}

void destroy_actor( void *arg ) {
  actor_destroy((actor_t*)arg);
}

void destroy_message( void *arg ) {
  message_destroy((message_t*)arg);
}

void actor_system_destroy(actor_system_t *actor_system) {
  fifo_each( actor_system->actors, &destroy_actor );
  fifo_empty( actor_system->actors );
  fifo_destroy( actor_system->actors );

  fifo_each( actor_system->message_pool, &destroy_message );
  fifo_empty( actor_system->message_pool );
  fifo_destroy( actor_system->message_pool );

  thread_pool_exit_all( actor_system->thread_pool );
  thread_pool_destroy( actor_system->thread_pool );
  free( actor_system );
}

message_t *actor_system_message_get( actor_system_t *actor_system, void *data, int type, actor_t *from ) {
  if ( !fifo_is_empty( actor_system->message_pool ) ) {
    message_t* msg = (message_t*) fifo_pop( actor_system->message_pool );
    msg->type = type;
    /* purposely keeping the original alloc'd message->id */
    msg->data = data;
    msg->promise = NULL;
    msg->from = from;
    return msg;
  }
  return message_create(data, type, from);
}

void actor_system_stop( actor_system_t * actor_system ) {
  thread_pool_exit_all( actor_system->thread_pool );
}

void actor_system_message_put(actor_system_t *actor_system, message_t *message) {
  fifo_push(actor_system->message_pool, message);
}

void actor_system_recycle_messages(actor_system_t *actor_system, fifo_t *message_fifo) {
  while (!fifo_is_empty( message_fifo )) {
    actor_system_message_put(actor_system, (message_t*)fifo_pop(message_fifo));
  }
}
