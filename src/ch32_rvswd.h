#ifndef CH32_RVSWD_H
#define CH32_RVSWD_H
#include <stdbool.h>
#include <stdint.h>
bool ch32_rvswd_start(void);
void ch32_rvswd_stop(void);
bool ch32_rvswd_read(uint8_t addr, uint32_t *value);
bool ch32_rvswd_write(uint8_t addr, uint32_t value);
#endif
