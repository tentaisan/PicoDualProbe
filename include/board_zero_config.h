/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2026 PicoDualProbe
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef BOARD_ZERO_H_
#define BOARD_ZERO_H_

// Temporary RP2040-Zero pin assignment. Change these definitions to suit wiring.
#define PROBE_IO_RAW
#define PROBE_CDC_UART

#include "zero_settings.h"
extern zero_settings_t zero_settings_active;
#define PROBE_SM 0
#define PROBE_PIN_OFFSET (zero_settings_active.swclk)
#define PROBE_PIN_SWCLK (zero_settings_active.swclk)
#define PROBE_PIN_SWDIO (zero_settings_active.swdio)
#define PROBE_PIN_TDI (zero_settings_active.tdi)
#define PROBE_PIN_TMS (zero_settings_active.tms)
#define PROBE_PIN_TDO (zero_settings_active.tdo)
#define PROBE_PIN_RESET (zero_settings_active.reset)
#define PROBE_UART_TX (zero_settings_active.uart_tx)
#define PROBE_UART_RX (zero_settings_active.uart_rx)
#define PROBE_UART_INTERFACE uart1
#define PROBE_UART_BAUDRATE 115200
#define PROBE_SWO_PIN (zero_settings_active.swo)
#define PROBE_LED_ACTIVE_HIGH (zero_settings_active.led_active_high)

#define PROBE_PRODUCT_STRING "PicoDualProbe Zero (CMSIS-DAP)"

#endif
