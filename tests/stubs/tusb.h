#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#define __unused __attribute__((unused))
#define DAP_PACKET_COUNT 8
#define DAP_PACKET_SIZE 64
#define TUSB_CLASS_VENDOR_SPECIFIC 255
#define TUSB_DIR_IN 1
#define TUSB_DIR_OUT 0
#define TU_VERIFY(c, r) do { if (!(c)) return (r); } while (0)
#define CFG_TUSB_DEBUG 0
typedef void *TaskHandle_t;
typedef struct { uint8_t bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol, bInterfaceNumber, bNumEndpoints; } tusb_desc_interface_t;
typedef struct { uint8_t bEndpointAddress; uint16_t packet_size; } tusb_desc_endpoint_t;
typedef struct { int unused; } tusb_control_request_t;
typedef enum { XFER_RESULT_SUCCESS, XFER_RESULT_FAILED } xfer_result_t;
static inline uint8_t tu_edpt_dir(uint8_t ep) { return !!(ep & 0x80); }
static inline uint16_t tu_edpt_packet_size(const tusb_desc_endpoint_t *ep) { return ep->packet_size; }
bool usbd_edpt_open(uint8_t, const tusb_desc_endpoint_t *);
bool usbd_edpt_xfer(uint8_t, uint8_t, uint8_t *, uint16_t);
#define portMAX_DELAY 0xffffffffu
#define eSetValueWithOverwrite 0
#define probe_info(...) ((void)0)
void xTaskNotify(TaskHandle_t, uint32_t, int);
void xTaskNotifyWait(uint32_t, uint32_t, uint32_t *, uint32_t);
