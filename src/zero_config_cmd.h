#ifndef ZERO_CONFIG_CMD_H
#define ZERO_CONFIG_CMD_H
#include <stdint.h>
#include <stdbool.h>
uint32_t zero_config_command(const uint8_t *request, uint8_t *response, bool dap_connected);
#endif
