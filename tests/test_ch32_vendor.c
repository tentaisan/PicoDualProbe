#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "DAP.h"
#include "ch32_transport.h"
struct test_dap_data DAP_Data;
uint32_t zero_config_command(const uint8_t *a,uint8_t *b,bool c) { (void)a;(void)b;(void)c; return 0; }
static int mode, stops;
uint32_t DAP_ProcessVendorCommand(const uint8_t *,uint8_t *);
bool ch32_transport_start(ch32_transport_mode_t m) { mode=m; return true; }
void ch32_transport_stop(void) { mode=0; stops++; }
bool ch32_transport_read(uint8_t a,uint32_t *v) { *v=0x12345678u; return mode && a==0x7f; }
bool ch32_transport_write(uint8_t a,uint32_t v) { return mode && a==0x7f && v==0x12345678u; }
int main(void) {
 uint8_t req[64]={ID_DAP_Vendor0,0}, resp[64]={0};
 assert(DAP_ProcessVendorCommand(req,resp)==((2u<<16)|2));
 assert(resp[0]==ID_DAP_Vendor0 && resp[1]==0 && mode==CH32_TRANSPORT_SWIO);
 req[1]=2; req[2]=0x7f;
 assert(DAP_ProcessVendorCommand(req,resp)==((3u<<16)|6));
 assert(resp[1]==0 && resp[2]==0x78 && resp[3]==0x56 && resp[4]==0x34 && resp[5]==0x12);
 req[1]=3; req[3]=0x78; req[4]=0x56; req[5]=0x34; req[6]=0x12;
 assert(DAP_ProcessVendorCommand(req,resp)==((7u<<16)|2) && resp[1]==0);
 DAP_Data.debug_port=1; req[1]=4; assert(DAP_ProcessVendorCommand(req,resp)==((2u<<16)|2) && resp[1]==1 && mode==CH32_TRANSPORT_SWIO);
 DAP_Data.debug_port=0; assert(DAP_ProcessVendorCommand(req,resp)==((2u<<16)|2) && resp[1]==0 && mode==CH32_TRANSPORT_RVSWD);
 req[1]=1; DAP_ProcessVendorCommand(req,resp); assert(resp[1]==0 && stops==1 && mode==0);
 req[1]=255; DAP_ProcessVendorCommand(req,resp); assert(resp[1]==1);
 puts("CH32 CMSIS-DAP vendor command tests passed");
}
