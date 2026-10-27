/*
 * Copyright (c) 2013-2017 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        1. December 2017
 * $Revision:    V2.0.0
 *
 * Project:      CMSIS-DAP Source
 * Title:        DAP_vendor.c CMSIS-DAP Vendor Commands
 *
 *---------------------------------------------------------------------------*/
 
#include "DAP_config.h"
#include "DAP.h"
#ifdef DEBUG_ON_ZERO
#include "ch32_transport.h"
#include "zero_config_cmd.h"
#endif

//**************************************************************************************************
/** 
\defgroup DAP_Vendor_Adapt_gr Adapt Vendor Commands
\ingroup DAP_Vendor_gr 
@{

The file DAP_vendor.c provides template source code for extension of a Debug Unit with 
Vendor Commands. Copy this file to the project folder of the Debug Unit and add the 
file to the MDK-ARM project under the file group Configuration.
*/

/** Process DAP Vendor Command and prepare Response Data
\param request   pointer to request data
\param response  pointer to response data
\return          number of bytes in response (lower 16 bits)
                 number of bytes in request (upper 16 bits)
*/

uint32_t DAP_ProcessVendorCommand(const uint8_t *request, uint8_t *response) {
  uint32_t num = (1U << 16) | 1U;

  *response++ = *request;        // copy Command ID

  switch (*request++) {          // first byte in request is Command ID
    case ID_DAP_Vendor0:
#ifdef DEBUG_ON_ZERO
      // Experimental CH32V003 SWIO transport. Status is 0 on success.
      {
        uint8_t op = *request++;
        bool ok = false;
        uint32_t value = 0;
        num += (1U << 16) | 1U; // operation byte and status
        if (op == 0) {
          if (DAP_Data.debug_port == DAP_PORT_DISABLED) { ok = ch32_transport_start(CH32_TRANSPORT_SWIO); }
        } else if (op == 1) {
          ch32_transport_stop(); ok = true;
        } else if (op == 2) {
          ok = ch32_transport_read(*request, &value);
          num += (1U << 16) | 4U;
        } else if (op == 3) {
          uint8_t addr = *request++;
          value = (uint32_t)request[0] | ((uint32_t)request[1] << 8) |
                  ((uint32_t)request[2] << 16) | ((uint32_t)request[3] << 24);
          ok = ch32_transport_write(addr, value);
          num += 5U << 16;
        }
        else if (op == 4) {
          if (DAP_Data.debug_port == DAP_PORT_DISABLED) { ok = ch32_transport_start(CH32_TRANSPORT_RVSWD); }
        }
        *response++ = ok ? 0 : 1;
        if (op == 2) {
          *response++ = (uint8_t)value; *response++ = (uint8_t)(value >> 8);
          *response++ = (uint8_t)(value >> 16); *response++ = (uint8_t)(value >> 24);
        }
      }
#endif
      break;

    case ID_DAP_Vendor1:
#ifdef DEBUG_ON_ZERO
      num += zero_config_command(request, response, DAP_Data.debug_port != DAP_PORT_DISABLED);
#endif
      break;
    case ID_DAP_Vendor2:  break;
    case ID_DAP_Vendor3:  break;
    case ID_DAP_Vendor4:  break;
    case ID_DAP_Vendor5:  break;
    case ID_DAP_Vendor6:  break;
    case ID_DAP_Vendor7:  break;
    case ID_DAP_Vendor8:  break;
    case ID_DAP_Vendor9:  break;
    case ID_DAP_Vendor10: break;
    case ID_DAP_Vendor11: break;
    case ID_DAP_Vendor12: break;
    case ID_DAP_Vendor13: break;
    case ID_DAP_Vendor14: break;
    case ID_DAP_Vendor15: break;
    case ID_DAP_Vendor16: break;
    case ID_DAP_Vendor17: break;
    case ID_DAP_Vendor18: break;
    case ID_DAP_Vendor19: break;
    case ID_DAP_Vendor20: break;
    case ID_DAP_Vendor21: break;
    case ID_DAP_Vendor22: break;
    case ID_DAP_Vendor23: break;
    case ID_DAP_Vendor24: break;
    case ID_DAP_Vendor25: break;
    case ID_DAP_Vendor26: break;
    case ID_DAP_Vendor27: break;
    case ID_DAP_Vendor28: break;
    case ID_DAP_Vendor29: break;
    case ID_DAP_Vendor30: break;
    case ID_DAP_Vendor31: break;
  }

  return (num);
}

///@}
