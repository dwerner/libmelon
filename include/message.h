#pragma once

#include "actor.h"
#include "promise.h"

typedef struct actor_t actor_t;
typedef struct message_t message_t;

struct message_t {
  int type; // map to user enum
  unsigned long id;
  void *data;
  const actor_t *from; // until I get some sleep, forward references are mystifying me with typedef struct members
  promise_t *promise;
};

message_t *message_create(void *data, int type, const actor_t *from);
void message_destroy(message_t *message);
