/**
 * @file main.c
 * @author vinay divakar
 * @brief main
 * @version 0.1
 * @date 2024-05-18
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "server_state_machine.h"

int main(int, char **) {
  while (1) {
    server_state_machine_init();
  }
  return 0;
}
