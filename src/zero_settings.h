#ifndef ZERO_SETTINGS_H
#define ZERO_SETTINGS_H
#include <stdbool.h>
#include <stdint.h>
/* Wire representation is deliberately byte-sized for the CMSIS-DAP vendor API. */
typedef struct {
    uint8_t swclk, swdio, tdi, tms, tdo, reset;
    uint8_t uart_tx, uart_rx, swo;
    uint8_t led[5]; /* error, JTAG, SWD, ready, reserved */
    uint8_t led_enabled; /* bit per role */
    uint8_t led_active_high;
    uint8_t swo_enabled;
} zero_settings_t;
enum zero_settings_error {
    ZERO_SETTINGS_OK, ZERO_SETTINGS_BAD_FLAG, ZERO_SETTINGS_BAD_PIN,
    ZERO_SETTINGS_PIN_CONFLICT, ZERO_SETTINGS_SWD_ORDER,
    ZERO_SETTINGS_UART_PINS, ZERO_SETTINGS_SWO_PIN
};
extern const zero_settings_t zero_settings_default;
extern zero_settings_t zero_settings_active;
enum zero_settings_error zero_settings_validate(const zero_settings_t *settings);
#define ZERO_SETTINGS_RECORD_SIZE 32u
enum zero_settings_load_result { ZERO_SETTINGS_LOADED, ZERO_SETTINGS_EMPTY, ZERO_SETTINGS_CORRUPT };
void zero_settings_encode_sequence(const zero_settings_t *settings, uint32_t sequence, uint8_t record[ZERO_SETTINGS_RECORD_SIZE]);
int zero_settings_select(const uint8_t *slot0, const uint8_t *slot1, zero_settings_t *settings, uint32_t *sequence);
void zero_settings_encode(const zero_settings_t *settings, uint8_t record[ZERO_SETTINGS_RECORD_SIZE]);
enum zero_settings_load_result zero_settings_decode(const uint8_t record[ZERO_SETTINGS_RECORD_SIZE], zero_settings_t *settings);

#endif
