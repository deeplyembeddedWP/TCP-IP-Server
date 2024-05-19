#ifndef __SERVER_H
#define __SERVER_H

#include "common.h"

#define SERVER_SOCKET_LISTEN_INDEX                                             \
  0 // listening socket index in poll fds descriptor set
#define SERVER_SOCKET_LISTEN_PORT_NUM                                          \
  12345 // listening socket port number to which clients request connection
#define SERVER_SOCKET_POLL_TIMEOUT                                             \
  -1 // poll timeout set to block indefinetly if not events occur
#define SERVER_CONNECTIONS_BACKLOG                                             \
  5 // maximum connections to be queued to be serviced
#define SERVER_STATE_MACHINE_FDS_MAX                                           \
  3 + 1 // maximum number of connections supported

int server_listen_begin(const uint16_t port);
int server_connections_accept(int fd, short int events,
                              int (*client_fd_add)(int, short int));
int server_read(int fd, uint8_t *recv_buff, size_t recv_buff_size);
int server_write(int fd, uint8_t *send_buff, size_t send_buff_size);

void server_recv_print(uint8_t *buffer, size_t data_size);

#endif //__SERVER_H
