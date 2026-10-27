/* Host test of the actual TinyUSB endpoint callbacks, with transport stubs. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/tusb_edpt_handler.c"
TaskHandle_t dap_taskhandle, tud_taskhandle;
void swo_trace_reset(void) {}
static unsigned calls, last_length;
static uint8_t *last_buffer;
bool usbd_edpt_open(uint8_t p, const tusb_desc_endpoint_t *e) { (void)p; (void)e; return true; }
bool usbd_edpt_xfer(uint8_t p, uint8_t e, uint8_t *b, uint16_t n) { (void)p; (void)e; calls++; last_length=n; last_buffer=b; return true; }
SemaphoreHandle_t xSemaphoreCreateMutex(void) { return (void *)1; }
void xSemaphoreGive(SemaphoreHandle_t s) { (void)s; }
void xSemaphoreTake(SemaphoreHandle_t s, unsigned n) { (void)s; (void)n; }
void vSemaphoreDelete(SemaphoreHandle_t s) { (void)s; }
void xTaskNotify(TaskHandle_t t, uint32_t n, int a) { (void)t; (void)n; (void)a; }
void xTaskNotifyWait(uint32_t a, uint32_t b, uint32_t *c, uint32_t d) { (void)a;(void)b;(void)c;(void)d; abort(); }
uint32_t DAP_ExecuteCommand(const uint8_t *a, uint8_t *b) { (void)a;(void)b; abort(); }
int main(void) {
 uint8_t request[64]={ID_DAP_Vendor1,2};
 assert(settings_packet_truncated(request,1));
 assert(settings_packet_truncated(request,18));
 assert(!settings_packet_truncated(request,19));
 request[1]=3; assert(settings_packet_truncated(request,3)); assert(!settings_packet_truncated(request,4));
 request[0]=ID_DAP_Info; assert(!settings_packet_truncated(request,1));
 dap_edpt_init(); dap_edpt_reset(0);
 buffer_t q = {0}; assert(buffer_empty(&q));
 for (unsigned i=0; i<7; i++) { assert(!buffer_full(&q)); q.wptr++; }
 assert(buffer_full(&q)); q.rptr++; assert(!buffer_full(&q)); q.wptr++;
 assert(buffer_full(&q)); q.rptr=8; assert(buffer_empty(&q));
 struct { tusb_desc_interface_t i; tusb_desc_endpoint_t ep[2]; } desc = {
  .i={255,0,0,1,2}, .ep={{4,64},{0x85,64}} };
 assert(!dap_edpt_open(0,&desc.i,sizeof(desc.i)));
 assert(dap_edpt_open(0,&desc.i,sizeof(desc)) == sizeof(desc.i)+2*sizeof(desc.ep[0]));
 assert(last_length==64);
 /* A short response, and exactly one full 64-byte packet, release the slot. */
 USBResponseBuffer.wptr=1; USBResponseBuffer.wasEmpty=true;
 assert(dap_edpt_xfer_cb(0,0x85,XFER_RESULT_SUCCESS,63)); assert(USBResponseBuffer.rptr==1);
 USBResponseBuffer.wptr=2; calls=0;
 assert(dap_edpt_xfer_cb(0,0x85,XFER_RESULT_SUCCESS,64)); assert(USBResponseBuffer.rptr==2 && calls==0);
 /* If endpoint MPS is smaller, an exact multiple needs a terminating ZLP. */
 _in_ep_packet_size=32; USBResponseBuffer.wptr=3; calls=0;
 assert(dap_edpt_xfer_cb(0,0x85,XFER_RESULT_SUCCESS,32));
 assert(calls==1 && last_length==0 && last_buffer==NULL && USBResponseBuffer.rptr==2);
 assert(dap_edpt_xfer_cb(0,0x85,XFER_RESULT_SUCCESS,0)); assert(USBResponseBuffer.rptr==3);
 assert(!dap_edpt_xfer_cb(0,0x85,XFER_RESULT_SUCCESS,65));
 /* A failed OUT must not enqueue a command, even if it reports bytes. */
 dap_edpt_reset(0); calls=0;
 USBRequestBuffer.data[0][0]=0x81;
 assert(dap_edpt_xfer_cb(0,4,XFER_RESULT_FAILED,5));
 assert(USBRequestBuffer.wptr==0 && calls==1 && last_length==64);
 /* Seven OUT slots are queued; the eighth completion applies backpressure. */
 dap_edpt_reset(0); calls=0;
 for (unsigned i=0;i<7;i++) assert(dap_edpt_xfer_cb(0,4,XFER_RESULT_SUCCESS,64));
 assert(USBRequestBuffer.wptr==7 && calls==7);
 assert(dap_edpt_xfer_cb(0,4,XFER_RESULT_SUCCESS,64));
 assert(USBRequestBuffer.wasFull && calls==7);
 unsigned gen=reset_generation; dap_edpt_reset(0);
 assert(reset_generation==gen+1 && buffer_empty(&USBRequestBuffer) && !USBRequestBuffer.wasFull);
 assert(!dap_edpt_xfer_cb(0,4,XFER_RESULT_SUCCESS,65));
 /* Reset during command execution must discard the old response. */
 dap_edpt_reset(0); USBRequestBuffer.wptr=1;
 USBRequestBuffer.data[0][0]=0x42; USBRequestBuffer.data_len[0]=1;
 uint8_t local[64], response[64]={0x42,0x99}; uint16_t len;
 gen=reset_generation;
 assert(copy_request(gen,local,&len) && len==1 && local[0]==0x42);
 dap_edpt_reset(0);
 USBRequestBuffer.wptr=1; USBRequestBuffer.data[0][0]=0x43;
 calls=0; assert(!publish_response(gen,response,2));
 assert(USBRequestBuffer.rptr==0 && USBResponseBuffer.wptr==0 && calls==0);
 gen=reset_generation; assert(publish_response(gen,response,2));
 assert(USBRequestBuffer.rptr==1 && USBResponseBuffer.wptr==1 && calls==1 && last_length==2);
 assert(USBResponseBuffer.data[0][1]==0x99);
 puts("USB endpoint tests passed");
}
