/**
 * @file server_state_machine.c
 * @author vinay divakar
 * @brief state machine to manage connections and file transfers
 * @version 0.1
 * @date 2024-05-17
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "server_state_machine.h"
#include "commands.h"
#include "file_transfer.h"
#include "packet.h"
#include "server.h"

static struct pollfd _fds[SERVER_STATE_MACHINE_FDS_MAX] = {};
static struct file_transfer_t _file_transfer[SERVER_STATE_MACHINE_FDS_MAX] = {};

/**
 * @brief resets the descriptor set
 */
static void _reset_descriptor_set(void) {
  for (int i = 0; i < sizeof(_fds) / sizeof(_fds[0]); i++) {
    _fds[i].fd = -1;
    _fds[i].events = 0;
  }
}

/**
 * @brief close all active connections & reset events
 */
static void _client_connections_clean_up(void) {
  for (int i = 0; i < sizeof(_fds) / sizeof(_fds[0]); i++) {
    if (_fds[i].fd >= 0)
      close(_fds[i].fd);
    _fds[i].events = 0;
  }
}

/**
 * @brief close an active connection & reset events
 *
 * @param[in] fds points to socket to be closed
 */
static void _client_connection_close(struct pollfd *fds) {
  if (fds->fd >= 0) {
    printf("client %d connection closed\r\n", fds->fd);
    close(fds->fd);
    fds->fd = -1;
    fds->events = 0;
    return;
  }
  printf("client %d connection already closed\r\n", fds->fd);
}

/**
 * @brief remove transfer context and close connection
 *
 * @param[in] fds points to socket to be closed
 */
static void
_client_connection_resources_release(struct pollfd *fd,
                                     struct file_transfer_t *list,
                                     size_t file_transfer_list_size) {
  file_transfer_context_remove(fd->fd, list, file_transfer_list_size);
  _client_connection_close(fd);
}

/**
 * @brief adds an accepted connection to the list for poll to monitor
 *
 * @param[in] fd points to connection to be added
 * @param[in] events events to be polled on this fd
 * @return 0 success, <0 error
 */
static int _client_connection_add(int fd, short int events) {
  int err = -ENOBUFS, on = 1;

  for (int i = SERVER_SOCKET_LISTEN_INDEX + 1;
       i < sizeof(_fds) / sizeof(_fds[0]); i++) {
    if (_fds[i].fd >= 0) {
      continue;
    }

    err = 0;
    _fds[i].fd = fd;
    _fds[i].events = events;

    err = ioctl(_fds[i].fd, FIONBIO, (char *)&on);
    if (err < 0) {
      printf("error %d errno %d client fd %d at idx %d ioctl\r\n", err, errno,
             _fds[i].fd, i);
      _client_connection_close(&_fds[i]);
    } else {
      printf("adding client fd %d, evt %hu at idx %d\r\n", _fds[i].fd,
             _fds[i].events, i);
    }
    break;
  }
  return err;
}

/**
 * @brief process events on all active connections
 * @return 0 success, <0 error
 */
static int _client_connection_events_process(void) {
  int err = 0;
  packet_t packet_rx = {};
  struct file_transfer_t transfer_ctx = {};
  for (int i = SERVER_SOCKET_LISTEN_INDEX + 1;
       i < sizeof(_fds) / sizeof(_fds[0]); i++) {
    err = 0;
    // check for errors
    if (_fds[i].revents & POLLNVAL) { // POLLNVAL
      printf("fd invalid, this should never happen unless theres a bug?\r\n");
      err = -ENOENT;
      break;
    } else if (_fds[i].revents & POLLERR ||
               _fds[i].revents & POLLHUP) { // an error occurred
      printf("poll error on fd %d at idx %d evt %hu\r\n", _fds[i].fd, i,
             _fds[i].revents);
      err = -EIO;
      goto CONNECTION_RELEASE;
    } else if (_fds[i].revents & POLLPRI) { // POLLPRI
      // TBD: To be understood and handled appropriately, for now lets ignore it
    } else if (_fds[i].revents & POLLIN) { // POLLIN
      printf("POLLIN on fd %d at idx %d\r\n", _fds[i].fd, i);
      // clear this event, poll will notify if we are able read again
      _fds[i].revents &= ~POLLIN;

      memset(&packet_rx, 0, sizeof(packet_rx));
      err = server_read(_fds[i].fd, packet_rx.data, PACKET_MAX_SIZE);
      if (err <= 0) { // an error occurred
        err = (err == 0) ? -ENETRESET : err;
        printf("client closed connection on fd %d, error %d\r\n", _fds[i].fd,
               err);
        goto CONNECTION_RELEASE;
      }

      if (packet_rx.packet_struct.cmd != CMD_DOWNLOAD_FILE) {
        // present we only support download service but can be
        // extended to support other services in the future
        printf("invalid command 0x%02X on fd %d\r\n",
               packet_rx.packet_struct.cmd, _fds[i].fd);
        err = -ENOMSG;
        goto CONNECTION_RELEASE;
      }

      memset(&transfer_ctx, 0, sizeof(transfer_ctx));

      transfer_ctx.client_fd = _fds[i].fd;

      size_t copy_size =
          (packet_rx.packet_struct.length < FILE_TRANSFER_NAME_SIZE_MAX)
              ? packet_rx.packet_struct.length
              : FILE_TRANSFER_NAME_SIZE_MAX;

      memcpy(transfer_ctx.filename, packet_rx.data + PACKET_HEADER_SIZE,
             copy_size);
      transfer_ctx.filename[copy_size] = '\0'; // null terminate it

      printf("fname: %s len:%ld\r\n", transfer_ctx.filename, copy_size);

      err = file_transfer_context_add(&transfer_ctx, _file_transfer,
                                      sizeof(_file_transfer) /
                                          sizeof(_file_transfer[0]));
      if (err < 0) { // context association successful?
        printf("error %d, context association for %d\r\n", err, _fds[i].fd);
        goto CONNECTION_RELEASE;
      }

      // enable POLLOUT so we can begin file transfer to this client
      _fds[i].events |= POLLOUT;

      // uncomment to enable for DBG
      // server_recv_print(packet_rx.data, packet_rx.packet_struct.length);

    CONNECTION_RELEASE:
      if (err < 0) {
        _client_connection_resources_release(&_fds[i], _file_transfer,
                                             sizeof(_file_transfer) /
                                                 sizeof(_file_transfer[0]));
      }
    } else if (_fds[i].revents & POLLOUT) { // POLLOUT
      // clear this event, poll will notify if we are able write again
      _fds[i].revents &= ~POLLOUT;
      // transfer file to this client in chunks
      err = file_transfer(_fds[i].fd, _file_transfer);
      if (err < 0) {
        printf("error file transfer %d\r\n", err);
        _client_connection_resources_release(&_fds[i], _file_transfer,
                                             sizeof(_file_transfer) /
                                                 sizeof(_file_transfer[0]));
      } else if (!err) { // transfer complete
        printf("transfer complete\r\n");
        // don't need send anything else until requested from client
        _fds[i].events = POLLIN;
      }
      break;
    } else if (_fds[i].revents == 0) { // no events
      // do nothing, since no events to be processed for this fd
    } else {
      // somethings not right
      printf("unexpected event %d on fd %d at idx %d\r\n", _fds[i].revents,
             _fds[i].fd, i);
      err = -ENOMSG;
      break;
    }
  } // for
  return err;
}

/**
 * @brief state machine to handle and manage transfers on active connections.
 */
void server_state_machine_init(void) {
  int err = 0;
  static enum server_state_t state = SERVER_LISTEN_BEGIN;

  switch (state) {
  case SERVER_LISTEN_BEGIN: { // listens for incoming commings
    _reset_descriptor_set();
    file_transfer_list_reset(_file_transfer, sizeof(_file_transfer) /
                                                 sizeof(_file_transfer[0]));

    state = SERVER_POLL_FOR_EVENTS;
    err = server_listen_begin(SERVER_SOCKET_LISTEN_PORT_NUM);
    if (err < 0) {
      state = SERVER_FATAL_ERROR;
    } else {
      _fds[SERVER_SOCKET_LISTEN_INDEX].fd = err;
      _fds[SERVER_SOCKET_LISTEN_INDEX].events = POLLIN;

      printf("server listening on port %hd using socket fd %d \r\n",
             SERVER_SOCKET_LISTEN_PORT_NUM,
             _fds[SERVER_SOCKET_LISTEN_INDEX].fd);
    }
  } break;

  case SERVER_POLL_FOR_EVENTS: { // polls for events on active sockets
    state = SERVER_POLL_INCOMING_CONNECTIONS;
    err = poll(_fds, SERVER_STATE_MACHINE_FDS_MAX, SERVER_SOCKET_POLL_TIMEOUT);
    if (err < 0) {
      printf("error %d errno %d polling\r\n", err, errno);
      state = SERVER_FATAL_ERROR;
    }
  } break;

  case SERVER_POLL_INCOMING_CONNECTIONS: { // accepts and manages incoming
                                           // connections
    state = SERVER_POLL_FOR_EVENTS;
    if (_fds[SERVER_SOCKET_LISTEN_INDEX].revents & POLLIN) {
      err = server_connections_accept(_fds[SERVER_SOCKET_LISTEN_INDEX].fd,
                                      POLLIN, _client_connection_add);
      // revents is not POLLIN, its an unexpected result
    } else if (_fds[SERVER_SOCKET_LISTEN_INDEX].revents &&
               _fds[SERVER_SOCKET_LISTEN_INDEX].revents != POLLIN) {
      printf("error %d accepting connection\r\n", err);
      state = SERVER_FATAL_ERROR;
    } else {
      state = SERVER_PROCESS_CONNECTION_EVENTS;
    }
  } break;

  case SERVER_PROCESS_CONNECTION_EVENTS: { // processes events on active
                                           // connections with client
    state = SERVER_POLL_FOR_EVENTS;
    err = _client_connection_events_process();
    if (err < 0) {
      state = SERVER_FATAL_ERROR;
    }
  } break;

  case SERVER_FATAL_ERROR: { // handles any unexpected errors
    printf("fatal error %d, clean up & exit\r\n", err);
    _client_connections_clean_up();
    exit(EXIT_FAILURE);
  } break;

  default:
    printf("should never get here, seems like a bug?\r\n");
    break;
  }
}