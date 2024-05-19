#ifndef __FILE_TRANSFER_H
#define __FILE_TRANSFER_H

#include "common.h"

#define FILE_TRANSFER_NAME_SIZE_MAX                                            \
  32 + 1 // Maximum size supported for the requested filename
#define FILE_TRANSFER_TABLE                                                    \
  "/home/vinay_divakar/file_storage" // files storage path
#define FILE_TRANSFER_PATH_NAME_SIZE_MAX                                       \
  64 // Maximum size supported for the file path
#define FILE_TRANSFER_BUFF_READ_SIZE                                           \
  32 //  Maximum size supported for buffer reads

struct file_transfer_t {
  int client_fd;            // identifier for this connection
  FILE *fp;                 // identifier for the file to be transferred
  size_t transferred_total; // total bytes transferred/read
  char filename[FILE_TRANSFER_NAME_SIZE_MAX]; // requested filename
};

void file_transfer_list_reset(struct file_transfer_t *list,
                              size_t file_transfer_list_size);
int file_transfer_context_add(struct file_transfer_t *ctx,
                              struct file_transfer_t *list,
                              size_t file_transfer_list_size);
void file_transfer_context_remove(int fd, struct file_transfer_t *file_transfer,
                                  size_t file_transfer_list_size);
int file_transfer(int fd, struct file_transfer_t *file_transfer);

#endif // __FILE_TRANSFER_H