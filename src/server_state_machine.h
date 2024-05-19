#ifndef __SERVER_STATE_MACHINE_H
#define __SERVER_STATE_MACHINE_H

#include "common.h"

enum server_state_t {
  SERVER_LISTEN_BEGIN,
  SERVER_POLL_FOR_EVENTS,
  SERVER_POLL_INCOMING_CONNECTIONS,
  SERVER_PROCESS_CONNECTION_EVENTS,
  SERVER_FATAL_ERROR
};

void server_state_machine_init(void);

#endif //__SERVER_H
