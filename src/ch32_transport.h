#ifndef CH32_TRANSPORT_H
#define CH32_TRANSPORT_H
#include <stdbool.h>
#include <stdint.h>
typedef enum { CH32_TRANSPORT_OFF, CH32_TRANSPORT_SWIO, CH32_TRANSPORT_RVSWD } ch32_transport_mode_t;
bool ch32_transport_start(ch32_transport_mode_t mode);
void ch32_transport_stop(void);
bool ch32_transport_is_active(void);
bool ch32_transport_read(uint8_t addr, uint32_t *value);
bool ch32_transport_write(uint8_t addr, uint32_t value);
#endif
