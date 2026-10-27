#pragma once
#define SWO_UART 1
#define SWO_BUFFER_SIZE 128
#define SWO_UART_MAX_BAUDRATE 2000000U

#define DAP_PACKET_SIZE 64

static inline unsigned TIMESTAMP_GET(void) { return 1234; }
