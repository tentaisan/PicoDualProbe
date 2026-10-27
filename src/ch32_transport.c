#include "ch32_transport.h"
#include "ch32_swio.h"
#include "ch32_rvswd.h"
static ch32_transport_mode_t active;
bool ch32_transport_is_active(void) { return active != CH32_TRANSPORT_OFF; }
void ch32_transport_stop(void) {
    if (active == CH32_TRANSPORT_SWIO) ch32_swio_stop();
    else if (active == CH32_TRANSPORT_RVSWD) ch32_rvswd_stop();
    active = CH32_TRANSPORT_OFF;
}
bool ch32_transport_start(ch32_transport_mode_t mode) {
    if (mode != CH32_TRANSPORT_SWIO && mode != CH32_TRANSPORT_RVSWD) return false;
    if (active == mode) return true;
    ch32_transport_stop();
    bool ok = mode == CH32_TRANSPORT_SWIO ? ch32_swio_start() : ch32_rvswd_start();
    if (ok) active = mode;
    return ok;
}
bool ch32_transport_read(uint8_t addr, uint32_t *value) {
    if (!value) return false;
    if (active == CH32_TRANSPORT_SWIO) return ch32_swio_read(addr, value);
    if (active == CH32_TRANSPORT_RVSWD) return ch32_rvswd_read(addr, value);
    return false;
}
bool ch32_transport_write(uint8_t addr, uint32_t value) {
    if (active == CH32_TRANSPORT_SWIO) return ch32_swio_write(addr, value);
    if (active == CH32_TRANSPORT_RVSWD) return ch32_rvswd_write(addr, value);
    return false;
}
