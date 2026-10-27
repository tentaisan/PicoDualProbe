#pragma once
#include "tusb.h"
typedef struct {
 void (*init)(void); bool (*deinit)(void); void (*reset)(uint8_t);
 uint16_t (*open)(uint8_t, const tusb_desc_interface_t *, uint16_t);
 bool (*control_xfer_cb)(uint8_t, uint8_t, const tusb_control_request_t *);
 bool (*xfer_cb)(uint8_t, uint8_t, xfer_result_t, uint32_t);
 void *sof;
} usbd_class_driver_t;
