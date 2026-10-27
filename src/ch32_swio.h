#ifndef CH32_SWIO_H
#define CH32_SWIO_H
#include <stdbool.h>
#include <stdint.h>
bool ch32_swio_start(void);
void ch32_swio_stop(void);
bool ch32_swio_read(uint8_t addr, uint32_t *value);
bool ch32_swio_write(uint8_t addr, uint32_t value);
#endif
