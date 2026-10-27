#include <assert.h>
#include <stdio.h>
#include "zero_settings.h"
int main(void) {
 zero_settings_t s=zero_settings_default;
 assert(zero_settings_validate(&s)==ZERO_SETTINGS_OK);
 s.led[0]=s.tdi; assert(zero_settings_validate(&s)==ZERO_SETTINGS_PIN_CONFLICT);
 s.led_enabled &= ~1u; assert(zero_settings_validate(&s)==ZERO_SETTINGS_OK);
 s=zero_settings_default; s.swdio=4; assert(zero_settings_validate(&s)==ZERO_SETTINGS_SWD_ORDER);
 s=zero_settings_default; s.uart_rx=13; assert(zero_settings_validate(&s)==ZERO_SETTINGS_UART_PINS);
 s=zero_settings_default; s.swo=0; assert(zero_settings_validate(&s)==ZERO_SETTINGS_SWO_PIN);
 s.swo_enabled=0; assert(zero_settings_validate(&s)==ZERO_SETTINGS_OK);
 s=zero_settings_default; s.tdi=16; assert(zero_settings_validate(&s)==ZERO_SETTINGS_BAD_PIN);
 s=zero_settings_default; s.led_enabled=32; assert(zero_settings_validate(&s)==ZERO_SETTINGS_BAD_FLAG);
 s=zero_settings_default; s.led_active_high=2; assert(zero_settings_validate(&s)==ZERO_SETTINGS_BAD_FLAG);
 assert(zero_settings_validate(NULL)==ZERO_SETTINGS_BAD_FLAG);
 uint8_t record[ZERO_SETTINGS_RECORD_SIZE]; zero_settings_t decoded;
 for (unsigned i=0;i<sizeof(record);i++) record[i]=0xff;
 assert(zero_settings_decode(record,&decoded)==ZERO_SETTINGS_EMPTY);
 s=zero_settings_default; zero_settings_encode(&s,record);
 assert(zero_settings_decode(record,&decoded)==ZERO_SETTINGS_LOADED && decoded.swclk==1);
 record[6]^=1; assert(zero_settings_decode(record,&decoded)==ZERO_SETTINGS_CORRUPT);
 s.led_active_high=2; zero_settings_encode(&s,record);
 assert(zero_settings_decode(record,&decoded)==ZERO_SETTINGS_CORRUPT);
 uint8_t a[32], b[32]; uint32_t seq=99;
 for (unsigned i=0;i<32;i++) a[i]=b[i]=0xff;
 assert(zero_settings_select(a,b,&decoded,&seq)==-1);
 s=zero_settings_default; zero_settings_encode(&s,b); /* legacy last sector */
 assert(zero_settings_select(a,b,&decoded,&seq)==1 && seq==0);
 s.led_active_high=0; zero_settings_encode_sequence(&s,1,a);
 assert(zero_settings_select(a,b,&decoded,&seq)==0 && seq==1 && !decoded.led_active_high);
 a[8]^=0x80; assert(zero_settings_select(a,b,&decoded,&seq)==1 && seq==0 && decoded.led_active_high);
 zero_settings_encode_sequence(&s,0xffffffffu,a); zero_settings_encode_sequence(&s,0,b);
 assert(zero_settings_select(a,b,&decoded,&seq)==1 && seq==0);
 b[0]^=1; a[0]^=1; assert(zero_settings_select(a,b,&decoded,&seq)==-2);
 /* Simulate power loss at every byte of the new page: old slot survives. */
 zero_settings_t next=zero_settings_default; next.led_active_high=0;
 zero_settings_encode_sequence(&zero_settings_default,41,b);
 uint8_t complete[32]; zero_settings_encode_sequence(&next,42,complete);
 for (unsigned n=0;n<32;n++) {
     for (unsigned i=0;i<32;i++) a[i]=i<n ? complete[i] : 0xff;
     assert(zero_settings_select(a,b,&decoded,&seq)==1 && seq==41 && decoded.led_active_high==1);
 }
 for (unsigned i=0;i<32;i++) a[i]=complete[i];
 assert(zero_settings_select(a,b,&decoded,&seq)==0 && seq==42 && decoded.led_active_high==0);
 puts("Zero configuration validation tests passed");
}
