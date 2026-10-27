#include "zero_settings.h"
zero_settings_t zero_settings_active;
const zero_settings_t zero_settings_default = {
    .swclk=1, .swdio=2, .tdi=3, .tms=6, .tdo=7, .reset=8,
    .uart_tx=4, .uart_rx=5, .swo=17,
    .led={9,10,11,12,13}, .led_enabled=0x1f,
    .led_active_high=1, .swo_enabled=1
};
/* GPIO16 is reserved for the onboard RGB LED. Some pins require soldering. */
static bool usable(uint8_t pin) { return pin < 30 && pin != 16; }
static bool uart1_tx(uint8_t p) { return p==4 || p==8 || p==20 || p==24; }
static bool uart1_rx(uint8_t p) { return p==5 || p==9 || p==21 || p==25; }
static bool uart0_rx(uint8_t p) { return p==1 || p==13 || p==17 || p==29; }
enum zero_settings_error zero_settings_validate(const zero_settings_t *s) {
    if (!s || s->led_enabled & ~0x1fu || s->led_active_high > 1 || s->swo_enabled > 1)
        return ZERO_SETTINGS_BAD_FLAG;
    if (s->swdio != s->swclk + 1u) return ZERO_SETTINGS_SWD_ORDER;
    if (!uart1_tx(s->uart_tx) || !uart1_rx(s->uart_rx)) return ZERO_SETTINGS_UART_PINS;
    if (s->swo_enabled && !uart0_rx(s->swo)) return ZERO_SETTINGS_SWO_PIN;
    uint8_t pins[14] = {s->swclk,s->swdio,s->tdi,s->tms,s->tdo,s->reset,
                        s->uart_tx,s->uart_rx,s->swo,s->led[0],s->led[1],
                        s->led[2],s->led[3],s->led[4]};
    uint32_t used=0;
    for (unsigned i=0;i<14;i++) {
        if ((i==8 && !s->swo_enabled) || (i>=9 && !(s->led_enabled & (1u<<(i-9))))) continue;
        if (!usable(pins[i])) return ZERO_SETTINGS_BAD_PIN;
        uint32_t bit=1u<<pins[i];
        if (used & bit) return ZERO_SETTINGS_PIN_CONFLICT;
        used |= bit;
    }
    return ZERO_SETTINGS_OK;
}

#include <string.h>
_Static_assert(sizeof(zero_settings_t)==17, "wire settings size changed");
static uint32_t crc32(const uint8_t *p, unsigned n) {
    uint32_t c=0xffffffffu;
    while (n--) { c^=*p++; for (unsigned k=0;k<8;k++) c=(c>>1) ^ (0xedb88320u & (0u-(c&1u))); }
    return ~c;
}
static void encode(const zero_settings_t *s, uint32_t seq, unsigned version, uint8_t record[ZERO_SETTINGS_RECORD_SIZE]) {
    memset(record,0,ZERO_SETTINGS_RECORD_SIZE);
    memcpy(record,"PDPC",4); record[4]=(uint8_t)version; record[5]=sizeof(*s);
    memcpy(record+6,s,sizeof(*s));
    for (unsigned i=0;i<4;i++) record[23+i]=(uint8_t)(seq>>(8*i));
    uint32_t c=crc32(record,28);
    for (unsigned i=0;i<4;i++) record[28+i]=(uint8_t)(c>>(8*i));
}
void zero_settings_encode(const zero_settings_t *s, uint8_t r[ZERO_SETTINGS_RECORD_SIZE]) { encode(s,0,1,r); }
void zero_settings_encode_sequence(const zero_settings_t *s, uint32_t seq, uint8_t r[ZERO_SETTINGS_RECORD_SIZE]) { encode(s,seq,2,r); }
enum zero_settings_load_result zero_settings_decode(const uint8_t r[ZERO_SETTINGS_RECORD_SIZE], zero_settings_t *s) {
    bool empty=true;
    for (unsigned i=0;i<ZERO_SETTINGS_RECORD_SIZE;i++) if (r[i]!=0xff) empty=false;
    if (empty) return ZERO_SETTINGS_EMPTY;
    uint32_t stored=(uint32_t)r[28] | (uint32_t)r[29]<<8 | (uint32_t)r[30]<<16 | (uint32_t)r[31]<<24;
    if (memcmp(r,"PDPC",4) || (r[4]!=1 && r[4]!=2) || r[5]!=sizeof(*s) || crc32(r,28)!=stored)
        return ZERO_SETTINGS_CORRUPT;
    zero_settings_t candidate;
    memcpy(&candidate,r+6,sizeof(candidate));
    if (zero_settings_validate(&candidate)!=ZERO_SETTINGS_OK) return ZERO_SETTINGS_CORRUPT;
    *s=candidate;
    return ZERO_SETTINGS_LOADED;
}

static uint32_t sequence_of(const uint8_t *r) {
    if (r[4]==1) return 0;
    return (uint32_t)r[23] | (uint32_t)r[24]<<8 | (uint32_t)r[25]<<16 | (uint32_t)r[26]<<24;
}
/* Return slot number, -1 for two empty slots, -2 for no valid slot. */
int zero_settings_select(const uint8_t *a,const uint8_t *b,zero_settings_t *out,uint32_t *sequence) {
    zero_settings_t x,y;
    enum zero_settings_load_result sa=zero_settings_decode(a,&x), sb=zero_settings_decode(b,&y);
    if (sa!=ZERO_SETTINGS_LOADED && sb!=ZERO_SETTINGS_LOADED)
        return sa==ZERO_SETTINGS_EMPTY && sb==ZERO_SETTINGS_EMPTY ? -1 : -2;
    int slot;
    if (sa!=ZERO_SETTINGS_LOADED) slot=1;
    else if (sb!=ZERO_SETTINGS_LOADED) slot=0;
    else { uint32_t delta=sequence_of(b)-sequence_of(a); slot=(delta && delta<0x80000000u) ? 1 : 0; }
    *out=slot ? y : x;
    if (sequence) *sequence=sequence_of(slot ? b : a);
    return slot;
}
