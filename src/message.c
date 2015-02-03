#include  <stdlib.h>

#include "message.h"
#include "logger.h"

static long messageId = 0;

// Actor system should be the only one repsonsible for creating, storing, retreiving or freeing messages
message_t *message_create(void *data, int type, const actor_t *from) {
  message_t *message = (message_t*) malloc( sizeof(message_t) );
  message->data = data;
  message->type = type;
  message->promise = NULL; // and why don't we create this here?
  message->from = from;
  message->id = ++messageId;
  if (message->id % 1000 == 0) {
    dna_log( DEBUG, "New message created. Reached new message id : %lu", message->id );
  }
  return message;
}

void message_destroy(message_t *message) {
  promise_destroy( message->promise );
  message->promise = NULL;
  free( message );
}

