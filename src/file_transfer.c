/**
 * @file file_transfer.c
 * @author vinay divakar
 * @brief manages file transfer list and transfer procedure to the client
 * @version 0.1
 * @date 2024-05-17
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "file_transfer.h"
#include "server.h"

/**
 * @brief reads data from a file
 *
 * @param[in] table points to directory containing the key
 * @param[in] key points to the key/filename in the table
 * @param[out] data buffer to be populated with read data
 * @param[in] size size of data to be read
 * @param[in] offset start offset to begin reading from
 * @param[in] fp file handler
 * @param[out] eof indicates EOF
 * @return number of bytes read >0 on success, 0 on EOF, <0 error
 */
static int _file_read(const char *table, const char *key, void *data,
                      size_t size, size_t offset, FILE *fp, bool *eof) {
  int err = 0, close_err = 0;

  char path[FILE_TRANSFER_PATH_NAME_SIZE_MAX] = {};
  snprintf(path, sizeof(path), "%s/%s", table, key);

  // printf("path: %s\r\n", path);

  do {
    fp = fopen(path, "rb");
    if (!fp) {
      err = -errno;
      printf("error fopen %d\r\n", errno);
      break;
    }

    err = fseek(fp, offset, SEEK_SET);
    if (err < 0) {
      err = -errno;
      printf("error fseek %d %d\r\n", err, errno);
      break;
    }

    err = fread(data, 1, size, fp);
    if (ferror(fp)) { // check of errors
      err = -EIO;
      clearerr(fp);
      printf("error fread\r\n");
    } else if (feof(fp)) { // check for EOF
      printf("reached EOF\r\n");
      *eof = true;
    } else if (err != size) {
      printf("read lesser %d than expected size %ld\r\n", err, size);
    }

    close_err = fclose(fp);
    if (close_err) {
      err = -errno;
      printf("error close %d\r\n", errno);
    }
  } while (0);

  return err;
}

/**
 * @brief resets the file transfer list
 *
 * @param[in] list points to the file transfer list
 * @param[in] file_transfer_list_size size of the list
 */
void file_transfer_list_reset(struct file_transfer_t *list,
                              size_t file_transfer_list_size) {
  for (size_t i = 0; i < file_transfer_list_size; i++) {
    list->client_fd = -1;
    list->fp = NULL;
    list->transferred_total = 0;
    list->filename[0] = '\0';
  }
}

/**
 * @brief adds a file transfer context to the list
 *
 * @param[in] ctx points to the file transfer context to be added
 * @param[in] list points to the file transfer list
 * @param[in] file_transfer_list_size size of the list
 * @return 0 success, <0 error
 */
int file_transfer_context_add(struct file_transfer_t *ctx,
                              struct file_transfer_t *list,
                              size_t file_transfer_list_size) {
  int err = -ENOBUFS;

  if (ctx->client_fd < 0) {
    printf("invalid fd %d\r\n", ctx->client_fd);
    return -EINVAL;
  } else if (ctx->filename[0] == '\0') {
    printf("invalid filename\r\n");
    return -ENODATA;
  }

  for (size_t i = 0; i < file_transfer_list_size; i++) {
    if (list[i].client_fd >= 0) {
      continue;
    }

    err = 0;
    // assosiate this connection to this context
    memcpy(&list[i], ctx, sizeof(struct file_transfer_t));
    printf("associated ctx to transfer list at idx %ld\r\n", i);
    break;
  }
  return err;
}

/**
 * @brief removes a file transfer context from the list
 *
 * @param[in] fd connection identifer whose context to be removed
 * @param[in] list points to the file transfer list
 * @param[in] file_transfer_list_size size of the list
 */
void file_transfer_context_remove(int fd, struct file_transfer_t *list,
                                  size_t file_transfer_list_size) {
  int err = -EINVAL;
  for (size_t i = 0; i < file_transfer_list_size; i++) {
    if (list[i].client_fd != fd) {
      continue;
    }

    err = 0;
    list[i].client_fd = -1;
    list[i].fp = NULL;
    list[i].transferred_total = 0;
    list[i].filename[0] = '\0';
    printf(
        "removed ctx association from transfer list for fd %d at idx %ld\r\n",
        fd, i);
  }

  if (err) {
    printf("an error may have occurred since fd %d does not exist in transfer "
           "list\r\n",
           fd);
  }
}

/**
 * @brief transfers the requested file to the client
 *
 * @param[in] fd connection over which transfer must happen
 * @param[in] file_transfer context associtated to this connection
 * @param[out] eof indicates EOF
 * @return number of bytes read >0 on success, 0 on EOF, <0 error
 */
int file_transfer(int fd, struct file_transfer_t *file_transfer) {
  int err = 0, send_result = 0;
  uint8_t buffer[FILE_TRANSFER_BUFF_READ_SIZE] = {};
  bool eof = false;

  do {
    err = _file_read(FILE_TRANSFER_TABLE, file_transfer->filename, buffer,
                     sizeof(buffer) / sizeof(buffer[0]),
                     file_transfer->transferred_total, file_transfer->fp, &eof);
    if (err < 0) {
      printf("error _file_read %d\r\n", err);
      break;
    }

    // check if we got something to be sent
    if (err) {
      send_result = server_write(fd, buffer, err);
    }

    // check if we managed to send anything
    if (send_result >= 0) {
      file_transfer->transferred_total += send_result;
      // enable to see whats being sent out
      // printf("content: %.*s\r\n", err, (char *)buffer);
    } else {
      err = send_result;
      printf("send error %d\r\n", err);
      break;
    }

    if (!err || eof) {                                    // check for EOF
      send_result = server_write(fd, (uint8_t *)&err, 1); // notify the client
      err = send_result < 0 ? send_result : 0;
      break;
    }
  } while (0);

  return err;
}
