#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "DAP_config.h"
#include "DAP.h"
#include "swo_trace.h"
uint32_t SWO_Transport(const uint8_t*,uint8_t*), SWO_Mode(const uint8_t*,uint8_t*);
uint32_t SWO_Baudrate(const uint8_t*,uint8_t*), SWO_Control(const uint8_t*,uint8_t*);
uint32_t SWO_Status(uint8_t*), SWO_Data(const uint8_t*,uint8_t*), SWO_ExtendedStatus(const uint8_t*,uint8_t*);
static int locked, enabled, capturing;
void swo_hw_lock(void) { assert(!locked); locked=1; }
void swo_hw_unlock(void) { assert(locked); locked=0; }
bool swo_hw_enable(void) { assert(locked); enabled++; return true; }
void swo_hw_disable(void) { assert(locked); enabled--; }
uint32_t swo_hw_baud(uint32_t x) { assert(locked && enabled); return x; }
void swo_hw_capture(bool b) { assert(locked && enabled); capturing=b; }
static void call(uint32_t (*fn)(const uint8_t*,uint8_t*),uint8_t value,uint8_t expected) {
 uint8_t r[64]={0}; assert(fn(&value,r)==((1u<<16)|1)); assert(r[0]==expected);
}
static void rx(unsigned b, bool f, bool o) { swo_hw_lock(); swo_trace_rx(b,f,o); swo_hw_unlock(); }
int main(void) {
 uint8_t b[64]={0}, req[4]={0};
 call(SWO_Control,1,DAP_ERROR); call(SWO_Transport,2,DAP_ERROR);
 call(SWO_Transport,1,DAP_OK); call(SWO_Mode,2,DAP_ERROR); call(SWO_Mode,1,DAP_OK); assert(enabled==1);
 req[0]=0; req[1]=0xc2; req[2]=1; SWO_Baudrate(req,b); assert(b[0]==0 && b[1]==0xc2 && b[2]==1);
 call(SWO_Control,1,DAP_OK); assert(capturing);
 call(SWO_Mode,0,DAP_ERROR); call(SWO_Transport,0,DAP_ERROR);
 for (unsigned i=0;i<130;i++) rx(i,false,false);
 SWO_Status(b); assert(b[0]==(DAP_SWO_CAPTURE_ACTIVE|DAP_SWO_BUFFER_OVERRUN) && b[1]==128);
 SWO_Status(b); assert(b[0]==DAP_SWO_CAPTURE_ACTIVE);
 req[0]=255; req[1]=0; assert((SWO_Data(req,b)&0xffff)==63); assert(b[1]==60);
 for (unsigned i=0;i<60;i++) assert(b[3+i]==i);
 for (unsigned i=0;i<60;i++) rx(i+130,false,false);
 assert((SWO_Data(req,b)&0xffff)==63); for (unsigned i=0;i<60;i++) assert(b[3+i]==i+60);
 assert((SWO_Data(req,b)&0xffff)==63); for (unsigned i=0;i<8;i++) assert(b[3+i]==i+120);
 for (unsigned i=0;i<52;i++) assert(b[11+i]==(uint8_t)(i+130));
 rx(0x55,true,true); SWO_Status(b); assert((b[0]&192)==192);
 req[0]=7; assert((SWO_ExtendedStatus(req,b)&0xffff)==13);
 assert(b[0]==DAP_SWO_CAPTURE_ACTIVE && b[1]==9 && b[5]==189 && b[9]==0xd2 && b[10]==4);
 call(SWO_Control,0,DAP_OK); assert(!capturing); rx(0x66,false,false);
 call(SWO_Mode,0,DAP_OK); assert(!enabled);
 call(SWO_Mode,1,DAP_OK); assert(enabled==1);
 req[0]=0; req[1]=0xc2; req[2]=1; SWO_Baudrate(req,b);
 call(SWO_Control,1,DAP_OK); rx(0x42,false,false);
 swo_trace_reset(); assert(!enabled && !capturing);
 SWO_Status(b); assert(!b[0] && !b[1]);
 call(SWO_Control,1,DAP_ERROR);
 puts("SWO command and ring tests passed");
}
