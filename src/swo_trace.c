/* CMSIS-DAP SWO command transport using a bounded interrupt-fed ring. */
#include <stdint.h>
#include <stdbool.h>
#include "DAP_config.h"
#include "DAP.h"
#include "swo_trace.h"
#if SWO_UART
#if (SWO_BUFFER_SIZE == 0) || (SWO_BUFFER_SIZE & (SWO_BUFFER_SIZE - 1))
#error SWO_BUFFER_SIZE must be a nonzero power of two
#endif
static uint8_t data[SWO_BUFFER_SIZE];
static uint32_t in, out;
static uint8_t transport, mode, active, errors;
static uint32_t rate, timestamp_index, timestamp_tick;
static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void put32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static uint8_t status(void) { uint8_t s = active | errors; errors = 0; return s; }
void swo_trace_rx(uint8_t b, bool framing, bool overrun) {
    if (!active) return;
    if (framing) errors |= DAP_SWO_STREAM_ERROR;
    if (overrun) errors |= DAP_SWO_BUFFER_OVERRUN;
    if (in - out == SWO_BUFFER_SIZE) { errors |= DAP_SWO_BUFFER_OVERRUN; return; }
    data[in++ & (SWO_BUFFER_SIZE-1)] = b;
    timestamp_index=in; timestamp_tick=TIMESTAMP_GET();
}
bool swo_trace_is_active(void) {
    swo_hw_lock(); bool result=active; swo_hw_unlock(); return result;
}
void swo_trace_reset(void) {
    swo_hw_lock();
    if (active) swo_hw_capture(false);
    active=0;
    if (mode==DAP_SWO_UART) swo_hw_disable();
    mode=transport=0; rate=in=out=errors=0; timestamp_index=timestamp_tick=0;
    swo_hw_unlock();
}
uint32_t SWO_Transport(const uint8_t *req, uint8_t *resp) {
    swo_hw_lock();
    bool ok = !active && req[0] <= 1;
    if (ok) transport = req[0];
    resp[0]=ok ? DAP_OK : DAP_ERROR;
    swo_hw_unlock(); return (1u<<16)|1u;
}
uint32_t SWO_Mode(const uint8_t *req, uint8_t *resp) {
    /* Disallow reconfiguration while receiving; require explicit stop first. */
    swo_hw_lock();
    bool ok = !active && (req[0]==DAP_SWO_OFF || req[0]==DAP_SWO_UART);
    if (ok && req[0] != mode) {
        if (mode==DAP_SWO_UART) swo_hw_disable();
        mode=DAP_SWO_OFF; rate=0;
        if (req[0]==DAP_SWO_UART && !swo_hw_enable()) ok=false;
        else mode=req[0];
    }
    resp[0]=ok ? DAP_OK : DAP_ERROR;
    swo_hw_unlock(); return (1u<<16)|1u;
}
uint32_t SWO_Baudrate(const uint8_t *req, uint8_t *resp) {
    uint32_t requested=le32(req), actual=0;
    swo_hw_lock();
    if (!active && mode==DAP_SWO_UART && requested && requested<=SWO_UART_MAX_BAUDRATE)
        actual=swo_hw_baud(requested);
    if (!active) rate=actual;
    put32(resp,actual); swo_hw_unlock(); return (4u<<16)|4u;
}
uint32_t SWO_Control(const uint8_t *req, uint8_t *resp) {
    uint8_t want=req[0]&DAP_SWO_CAPTURE_ACTIVE;
    swo_hw_lock();
    bool ok = req[0]<=1 && (!want || (mode==DAP_SWO_UART && rate && transport==1));
    if (ok && want!=active) {
        if (want) { in=out=0; errors=0; timestamp_index=timestamp_tick=0; active=1; swo_hw_capture(true); }
        else { swo_hw_capture(false); active=0; }
    }
    resp[0]=ok ? DAP_OK : DAP_ERROR;
    swo_hw_unlock(); return (1u<<16)|1u;
}
uint32_t SWO_Status(uint8_t *resp) {
    swo_hw_lock(); resp[0]=status(); put32(resp+1,in-out); swo_hw_unlock(); return 5;
}
uint32_t SWO_ExtendedStatus(const uint8_t *req, uint8_t *resp) {
    uint32_t n=0;
    swo_hw_lock();
    if (req[0]&1) resp[n++]=status();
    if (req[0]&2) { put32(resp+n,in-out); n+=4; }
    if (req[0]&4) { put32(resp+n,timestamp_index); put32(resp+n+4,timestamp_tick); n+=8; }
    swo_hw_unlock(); return (1u<<16)|n;
}
uint32_t SWO_Data(const uint8_t *req, uint8_t *resp) {
    uint32_t wanted=(uint32_t)req[0] | ((uint32_t)req[1]<<8);
    swo_hw_lock();
    uint32_t count=transport==1 ? in-out : 0;
    if (count>wanted) count=wanted;
    if (count>DAP_PACKET_SIZE-4) count=DAP_PACKET_SIZE-4;
    resp[0]=status(); resp[1]=(uint8_t)count; resp[2]=(uint8_t)(count>>8);
    for (uint32_t i=0;i<count;i++) resp[3+i]=data[out++ & (SWO_BUFFER_SIZE-1)];
    swo_hw_unlock(); return (2u<<16)|(3u+count);
}
#endif
