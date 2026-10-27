/* WCH QingKe V4 RVSWD framing, informed by the MIT-licensed implementations:
 * https://github.com/ImproperCatGirl/RVSWD_pico
 * https://github.com/Nicolai-Electronics/esp32-component-rvswd
 * See CH32_RVSWD_LICENSE.txt for the Nicolai Electronics license notice. */
#include "ch32_rvswd.h"
#include "probe_config.h"
#ifdef DEBUG_ON_ZERO
#include "pico/stdlib.h"
// Bit timing validated with the CH32V203K8T6.
#define CH32_RVSWD_HALF_PERIOD_US 2u
static bool active;
static void delay(void) { busy_wait_us_32(CH32_RVSWD_HALF_PERIOD_US); }
static void data(bool bit) {
    gpio_put(PROBE_PIN_SWDIO, 0);
    gpio_set_dir(PROBE_PIN_SWDIO, bit ? GPIO_IN : GPIO_OUT);
}
static void clock_bit(bool bit) {
    gpio_put(PROBE_PIN_SWCLK, 0); data(bit); delay();
    gpio_put(PROBE_PIN_SWCLK, 1); delay();
}
static bool read_bit(void) {
    gpio_put(PROBE_PIN_SWCLK, 0); data(true); delay();
    gpio_put(PROBE_PIN_SWCLK, 1); delay(); return gpio_get(PROBE_PIN_SWDIO);
}
static void start_frame(void) {
    data(true); gpio_put(PROBE_PIN_SWCLK, 1); delay();
    data(false); delay(); gpio_put(PROBE_PIN_SWCLK, 0); delay();
}
static void stop_frame(void) {
    gpio_put(PROBE_PIN_SWCLK, 0); data(false); delay();
    gpio_put(PROBE_PIN_SWCLK, 1); delay(); data(true); delay();
}
static void bits(uint32_t value, unsigned n) {
    while (n--) clock_bit((value >> n) & 1u);
}
static bool parity(uint32_t value) { return __builtin_parity(value); }
static void header(uint8_t addr, bool write) {
    start_frame(); bits(addr, 7); clock_bit(write);
    clock_bit(parity(addr) ^ write); bits(0x15, 5);
}
bool ch32_rvswd_start(void) {
    if (active) return true;
    gpio_init(PROBE_PIN_SWCLK); gpio_put(PROBE_PIN_SWCLK, 1);
    gpio_set_dir(PROBE_PIN_SWCLK, GPIO_OUT);
    gpio_init(PROBE_PIN_SWDIO); gpio_pull_up(PROBE_PIN_SWDIO); data(true);
    for (unsigned i = 0; i < 100; i++) {
        gpio_put(PROBE_PIN_SWCLK, 0); delay(); gpio_put(PROBE_PIN_SWCLK, 1); delay();
    }
    stop_frame(); active = true; return true;
}
void ch32_rvswd_stop(void) {
    if (!active) return;
    gpio_deinit(PROBE_PIN_SWCLK); gpio_deinit(PROBE_PIN_SWDIO); active = false;
}
bool ch32_rvswd_read(uint8_t addr, uint32_t *value) {
    if (!active || addr > 0x7f || !value) return false;
    header(addr, false); uint32_t v = 0;
    for (unsigned i = 0; i < 32; i++) v = (v << 1) | read_bit();
    bool received_parity = read_bit(); bits(0x17, 5); stop_frame();
    *value = v; return received_parity == parity(v);
}
bool ch32_rvswd_write(uint8_t addr, uint32_t value) {
    if (!active || addr > 0x7f) return false;
    header(addr, true); bits(value, 32); clock_bit(parity(value));
    bits(0x17, 5); stop_frame(); return true;
}
#else
bool ch32_rvswd_start(void) { return false; }
void ch32_rvswd_stop(void) {}
bool ch32_rvswd_read(uint8_t a, uint32_t *v) { (void)a; (void)v; return false; }
bool ch32_rvswd_write(uint8_t a, uint32_t v) { (void)a; (void)v; return false; }
#endif
