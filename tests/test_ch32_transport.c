#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "ch32_transport.h"
static int sw_start, sw_stop, rv_start, rv_stop, sw_read, rv_read, sw_write, rv_write;
static bool fail_start;
bool ch32_swio_start(void) { sw_start++; return !fail_start; }
void ch32_swio_stop(void) { sw_stop++; }
bool ch32_rvswd_start(void) { rv_start++; return !fail_start; }
void ch32_rvswd_stop(void) { rv_stop++; }
bool ch32_swio_read(uint8_t a, uint32_t *v) { sw_read++; *v=a; return true; }
bool ch32_rvswd_read(uint8_t a, uint32_t *v) { rv_read++; *v=a; return true; }
bool ch32_swio_write(uint8_t a, uint32_t v) { (void)a; (void)v; sw_write++; return true; }
bool ch32_rvswd_write(uint8_t a, uint32_t v) { (void)a; (void)v; rv_write++; return true; }
int main(void) {
 uint32_t value;
 assert(!ch32_transport_read(1,&value)); assert(!ch32_transport_write(1,2));
 assert(!ch32_transport_start(CH32_TRANSPORT_OFF));
 assert(ch32_transport_start(CH32_TRANSPORT_SWIO));
 assert(ch32_transport_start(CH32_TRANSPORT_SWIO) && sw_start==1);
 assert(ch32_transport_read(7,&value) && value==7 && sw_read==1);
 assert(!ch32_transport_read(1,NULL)); assert(ch32_transport_write(1,2) && sw_write==1);
 assert(ch32_transport_start(CH32_TRANSPORT_RVSWD)); assert(sw_stop==1 && rv_start==1);
 assert(ch32_transport_read(9,&value) && value==9 && rv_read==1);
 assert(ch32_transport_write(1,2) && rv_write==1);
 fail_start=true; assert(!ch32_transport_start(CH32_TRANSPORT_SWIO));
 assert(rv_stop==1 && !ch32_transport_read(1,&value));
 ch32_transport_stop(); assert(rv_stop==1 && sw_stop==1);
 puts("CH32 transport switching tests passed");
}
