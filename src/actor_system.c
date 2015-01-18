#include "actor_system.h"
#include "promise.h"
#include "actor.h"

actor_system_t *actor_system_create(const char* name){
  actor_system_t *actor_system = (actor_system_t*) malloc( sizeof(actor_system_t) );
  actor_system->name = name;
  actor_system->actors = fifo_create("actors", 0);
  actor_system->thread_pool = thread_pool_create("actor system thread pool", 8);
  return actor_system;
}

void actor_system_add(actor_system_t *actor_system, actor_t *actor) {
  actor->pool = actor_system->thread_pool;
  fifo_push( actor_system->actors, actor );
}

void spawn_actor( void *arg ) {
  actor_t *actor = (actor_t*)arg;
  if (actor && actor->pool) {
    actor_spawn(actor);
  }
}

void actor_system_run(actor_system_t *actor_system) {
  fifo_each( actor_system->actors, &spawn_actor );
}

void destroy_actor( void *arg ) { actor_destroy((actor_t*)arg); }

void actor_system_destroy(actor_system_t *actor_system) {
  fifo_each( actor_system->actors, &destroy_actor );
  fifo_empty( actor_system->actors );
  fifo_destroy( actor_system->actors );
  thread_pool_destroy( actor_system->thread_pool );
  free( actor_system );
}
