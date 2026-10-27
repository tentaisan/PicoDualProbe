/* SWIO physical transport adapted from PicoRVD, MIT: CH32_SWIO_LICENSE.txt. */
#include "ch32_swio.h"
#include "probe_config.h"
#ifdef DEBUG_ON_ZERO
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "ch32_swio.pio.h"

// PIO transactions and timing are based on the validated PicoRVD SWIO waveform.
#define CH32_SWIO_TIMEOUT_MS 100
#define CH32_SWIO_PIO_HZ 10416667.0f
static int sm = -1;
static uint offset;
static bool put(uint32_t data) {
    absolute_time_t end = make_timeout_time_ms(CH32_SWIO_TIMEOUT_MS);
    while (pio_sm_is_tx_fifo_full(pio1, sm)) {
        if (absolute_time_diff_us(get_absolute_time(), end) <= 0) return false;
        tight_loop_contents();
    }
    pio_sm_put(pio1, sm, data);
    return true;
}

void ch32_swio_stop(void) {
    if (sm < 0) return;
    pio_sm_set_enabled(pio1, sm, false);
    pio_remove_program(pio1, &singlewire_program, offset);
    pio_sm_unclaim(pio1, sm);
    gpio_deinit(PROBE_PIN_SWDIO);
    sm = -1;
}

bool ch32_swio_start(void) {
    if (sm >= 0) return true;
    sm = pio_claim_unused_sm(pio1, false);
    if (sm < 0) return false;
    if (!pio_can_add_program(pio1, &singlewire_program)) {
        pio_sm_unclaim(pio1, sm); sm = -1; return false;
    }
    offset = pio_add_program(pio1, &singlewire_program);
    const uint pin = PROBE_PIN_SWDIO;
    gpio_init(pin); gpio_put(pin, 0); gpio_set_dir(pin, GPIO_OUT);
    sleep_us(8); gpio_set_dir(pin, GPIO_IN);
    gpio_set_drive_strength(pin, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_slew_rate(pin, GPIO_SLEW_RATE_SLOW);
    pio_gpio_init(pio1, pin);
    pio_sm_config cfg = singlewire_program_get_default_config(offset);
    sm_config_set_sideset(&cfg, 1, false, true);
    sm_config_set_out_pins(&cfg, pin, 1);
    sm_config_set_in_pins(&cfg, pin);
    sm_config_set_set_pins(&cfg, pin, 1);
    sm_config_set_sideset_pins(&cfg, pin);
    sm_config_set_out_shift(&cfg, false, false, 32);
    sm_config_set_in_shift(&cfg, false, true, 32);
    sm_config_set_clkdiv(&cfg, (float)clock_get_hz(clk_sys) / CH32_SWIO_PIO_HZ);
    pio_sm_init(pio1, sm, offset, &cfg);
    pio_sm_set_pins(pio1, sm, 0);
    pio_sm_set_enabled(pio1, sm, true);
    if (!ch32_swio_write(0x7e, 0x5aa50400) ||
        !ch32_swio_write(0x7d, 0x5aa50400)) {
        ch32_swio_stop(); return false;
    }
    return true;
}

bool ch32_swio_read(uint8_t addr, uint32_t *value) {
    if (sm < 0 || addr > 0x7f || !put(((~(uint32_t)addr) << 1) | 1u)) return false;
    absolute_time_t end = make_timeout_time_ms(CH32_SWIO_TIMEOUT_MS);
    while (pio_sm_is_rx_fifo_empty(pio1, sm)) {
        if (absolute_time_diff_us(get_absolute_time(), end) <= 0) return false;
        tight_loop_contents();
    }
    *value = pio_sm_get(pio1, sm);
    return true;
}

bool ch32_swio_write(uint8_t addr, uint32_t value) {
    return sm >= 0 && addr <= 0x7f && put((~(uint32_t)addr) << 1) && put(~value);
}
#else
bool ch32_swio_start(void) { return false; }
void ch32_swio_stop(void) {}
bool ch32_swio_read(uint8_t addr, uint32_t *value) { (void)addr; (void)value; return false; }
bool ch32_swio_write(uint8_t addr, uint32_t value) { (void)addr; (void)value; return false; }
#endif
