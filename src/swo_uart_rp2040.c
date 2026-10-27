/* RP2040 UART0 receiver for SWO. UART1 remains dedicated to USB CDC. */
#include "probe_config.h"
#ifdef DEBUG_ON_ZERO
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/critical_section.h"
#include "hardware/uart.h"
#include "hardware/regs/uart.h"
#include "swo_trace.h"
static critical_section_t lock;
static bool initialized;
static void swo_irq(void);
void swo_hw_init(void) {
    critical_section_init(&lock);
    irq_set_exclusive_handler(UART0_IRQ,swo_irq);
    irq_set_enabled(UART0_IRQ,true);
    initialized=true;
}
void swo_hw_lock(void) { critical_section_enter_blocking(&lock); }
void swo_hw_unlock(void) { critical_section_exit(&lock); }
static void swo_irq(void) {
    uart_hw_t *hw=uart_get_hw(uart0);
    while (!(hw->fr & UART_UARTFR_RXFE_BITS)) {
        uint32_t v=hw->dr;
        critical_section_enter_blocking(&lock);
        swo_trace_rx((uint8_t)v, !!(v & (UART_UARTDR_FE_BITS | UART_UARTDR_PE_BITS | UART_UARTDR_BE_BITS)),
                     !!(v & UART_UARTDR_OE_BITS));
        critical_section_exit(&lock);
        if (v & (UART_UARTDR_OE_BITS | UART_UARTDR_FE_BITS | UART_UARTDR_PE_BITS | UART_UARTDR_BE_BITS)) hw->rsr=0;
    }
}
bool swo_hw_enable(void) {
    if (!initialized || !zero_settings_active.swo_enabled) return false;
    uart_init(uart0, 115200);
    uart_set_format(uart0,8,1,UART_PARITY_NONE);
    uart_set_fifo_enabled(uart0,true);
    gpio_set_function(PROBE_SWO_PIN,GPIO_FUNC_UART);
    return true;
}
void swo_hw_disable(void) {
    uart_set_irq_enables(uart0,false,false);
    uart_deinit(uart0);
    gpio_deinit(PROBE_SWO_PIN);
}
uint32_t swo_hw_baud(uint32_t requested) { return uart_set_baudrate(uart0,requested); }
void swo_hw_capture(bool enabled) {
    uart_set_irq_enables(uart0,false,false);
    if (enabled) {
        uart_hw_t *hw=uart_get_hw(uart0);
        while (!(hw->fr & UART_UARTFR_RXFE_BITS)) (void)hw->dr;
        hw->rsr=0;
        uart_set_irq_enables(uart0,true,false);
    }
}
#endif
