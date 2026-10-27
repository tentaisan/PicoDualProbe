#ifndef SWO_TRACE_H
#define SWO_TRACE_H
#include <stdint.h>
#include <stdbool.h>
/* Platform supplies mutual exclusion between DAP calls and receiver interrupt. */
void swo_hw_init(void);
void swo_trace_reset(void);
bool swo_trace_is_active(void);
void swo_hw_lock(void);
void swo_hw_unlock(void);
bool swo_hw_enable(void);
void swo_hw_disable(void);
uint32_t swo_hw_baud(uint32_t requested);
void swo_hw_capture(bool enabled);
/* Called under the platform lock by the receiver. */
void swo_trace_rx(uint8_t data, bool framing_error, bool overrun);
#endif
