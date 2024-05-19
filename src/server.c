/**
 * @file server.c
 * @author vinay divakar
 * @brief socket configurations and operations
 * @version 0.1
 * @date 2024-05-17
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "server.h"

/**
 * @brief configures socket to listen for connections
 *
 * @param[in] port port used for listening
 * @return 0 success, <0 error
 */
static int _listen(const uint16_t port) {
  int err = 0, fd = -1, on = 1;

  do {
    err = socket(AF_INET, SOCK_STREAM, 0);
    if (err < 0) {
      printf("error %d create socket\r\n", err);
      break;
    }
    fd = err;

    err = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    if (err < 0) {
      printf("error %d setsockopt\r\n", err);
      break;
    }

    // configure socket to be non-blocking
    err = ioctl(fd, FIONBIO, (char *)&on);
    if (err < 0) {
      printf("error %d ioctl\r\n", err);
      break;
    }

    struct sockaddr_in _address = {};

    // setup address to be bound
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(port);

    err = bind(fd, (struct sockaddr *)&_address, sizeof(_address));
    if (err < 0) {
      printf("error %d bind socket\r\n", err);
      break;
    }

    err = listen(fd, SERVER_CONNECTIONS_BACKLOG);
    if (err < 0) {
      printf("error %d listen socket\r\n", err);
      break;
    }
  } while (0);

  if (err && fd >= 0) {
    close(fd);
  }

  return err < 0 ? -errno : fd;
}

/**
 * @brief begins listening for connections
 *
 * @param[in] port port used for listening f
 * @return 0 success, <0 error
 */
int server_listen_begin(const uint16_t port) {
  int err = 0, fd = -1;
  do {
    err = _listen(port);
    if (err < 0) {
      printf("error %d unable to setup listen\r\n", err);
      break;
    }
    fd = err;
  } while (0);

  return err < 0 ? err : fd;
}

/**
 * @brief accepts incoming connections
 *
 * @param[in] fd incoming connection handler
 * @param[in] events revents to poll for this connection
 * @param[in] client_fd_add callback to add connections to the polling list
 * @return 0 success, <0 error
 */
int server_connections_accept(int fd, short int events,
                              int (*client_fd_add)(int, short int)) {
  int err = 0, fd_new = -1;
  struct sockaddr_in _address = {};
  int _address_len = sizeof(_address);

  do {
    int fd_new =
        accept(fd, (struct sockaddr *)&_address, (socklen_t *)&_address_len);
    if (fd_new < 0) {
      err = (errno == EWOULDBLOCK) ? 0 : -errno;
      break;
    }

    client_fd_add(fd_new, events);

  } while (fd_new != -1);

  return err;
}

/**
 * @brief reads from the socket
 *
 * @param[in] fd connection handler
 * @param[out] recv_buff points to the buffer to be populated with recv data
 * @param[in] recv_buff_size size of the recv buffer
 * @return number of bytes read or <0 on error
 */
int server_read(int fd, uint8_t *recv_buff, size_t recv_buff_size) {
  int result = 0, total_recieved = 0, received = 0;
  size_t offset = 0;

  do {
    received = recv(fd, recv_buff + offset, recv_buff_size - offset, 0);
    if (received == 0) {
      result = total_recieved;
      break;
    } else if (received < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        result = total_recieved;
        break;
      }
      result = -errno;
      break;
    } else {
      total_recieved += received;
      offset += received;

      if (offset >= recv_buff_size) {
        offset = 0;
      }
    }
  } while (true);

  return result;
}

/**
 * @brief sends data over socket connection
 *
 * @param[in] fd connection handler
 * @param[in] send_buff points to buffer to be sent
 * @param[in] send_buff_size size of data to send
 * @return number of bytes on success, 0 on would block, <0 error
 */
int server_write(int fd, uint8_t *send_buff, size_t send_buff_size) {
  int err = 0;

  err = send(fd, send_buff, send_buff_size, 0);
  if (err < 0) {
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
      // indicate no bytes were sent
      err = 0; // try again once client is ready to receive
    } else {
      err = -errno;
    }
  }
  return err;
}

/**
 * @brief prints the received data
 *
 * @param[in] buffer points to buffer to be dumped
 * @param[in] data_size  size of the data
 * @return 0 success, <0 error
 */
void server_recv_print(uint8_t *buffer, size_t data_size) {
  printf("recv: ");
  for (size_t i = 0; i < data_size; i++) {
    printf("0x%02X ", buffer[i]);
  }
  printf("\r\n");
}
