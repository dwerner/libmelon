# libmelon

An actor model library in plain C.

This is a bit of a play-project, meant to be a return to OOP-style C (C11 in this case). It represents my forays into developing a coherent style in C, learning pthreads, and developing my own datastructures to represent the library's primitives.

Sample construction of an actor:

```c
...

/* Define some message types */
typedef enum {
  PING = 0,
  PONG = 1,
  DONE = 2
} message_type_t;

...

/* Our user-defined 'receive' method. This represents the message processing for a given actor.
   Must return: NULL or a promise_t -> a resolved promise with a value, which can be a chain of promises.
   An immediately resolved promise can be created with promise_resolved()
   actor_send() returns a promise */

promise_t *actor_pong_receive( actor_t *this, message_t *msg ) {
  switch( msg->type ) {

    case PONG: {
      if (msg->id % 100000 == 0) {
        dna_log(DEBUG, "%s-> PONG ->%s %lu", this->name, msg->from->name, msg->id);
      }
      message_t *response = actor_message_create(this, NULL, (msg->id < TEST_MESSAGE_COUNT ? PING : DONE) );
      response->id = msg->id + 1;
      return actor_send(msg->from, response);
    };

    case DONE: {
      message_t *response = actor_message_create(this, NULL,  DONE );
      response->id = msg->id + 1;
      dna_log(DEBUG, "- received DONE : %s-> DONE ->%s %lu", this->name, msg->from->name, msg->id);
      actor_kill( (actor_t*)this, NULL );
      actor_kill( (actor_t*)msg->from, NULL );
      return promise_resolved(response);
    };

    /* If this actor did not understand the message it was sent, it will return NULL,
       and the original promise will be resolved as NULL. */

    default: break;
  };

  return NULL;
}

...
/* useage example (main) */

/* Create an actor system */
actor_system_t *actor_system = actor_system_create("stuff");

/* create a couple of actors */
actor_t *actor1 = actor_create( &actor_ping_receive, "ping" );
actor_t *actor2 = actor_create( &actor_pong_receive, "pong" );

/* add those actors to the system */
actor_system_add( actor_system, actor1 );
actor_system_add( actor_system, actor2 );

/* create a message to send to one of the actors */
message_t *message = actor_message_create( actor2, NULL, PING );
message->id = 1;

/* 'send' a message to one of the actors. This operation's result is a 'promise', essentially a blocking queue with a single element. */
promise_t *promise = actor_send( actor1, message );

/* start processing in the actor system */
actor_system_run( actor_system );

/* block this thread (the main thread) until the promise can be resolved */
void *val = promise_get( promise );
if (val) {
	message_t *response = (message_t*)val;
	dna_log(DEBUG, "resolved promise: %s", (response->type == DONE ? "PASSED" : "FAILED") );
}

/* kill off the running actors (optional) */
actor_kill( actor1, NULL );
actor_kill( actor2, NULL );

/* destroy the actor system */
actor_system_destroy( actor_system );
```

# Build instructions

`mkdir build && cd build && cmake .. && make`

Should compile on most linux systems with gcc installed. Dependency on pthreads and check
