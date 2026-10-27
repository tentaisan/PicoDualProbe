/* RAM-only UART test source, not a real CoreSight SWO producer.
 * Pico GP0 -> Zero SWO input. SWO_TEST_BAUD 8N1, ascending 00..ff. */
#include "pico/stdlib.h"
#include "hardware/uart.h"
#ifndef SWO_TEST_BAUD
#define SWO_TEST_BAUD 115200
#endif
int main(void) {
    uart_init(uart0,SWO_TEST_BAUD);
    gpio_set_function(0,GPIO_FUNC_UART);
    for (;;) {
        for (unsigned i=0;i<256;i++) uart_putc_raw(uart0,(uint8_t)i);
        sleep_ms(2);
    }
}
