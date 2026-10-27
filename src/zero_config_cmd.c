#include <string.h>
#ifdef DEBUG_ON_ZERO
#include "zero_settings.h"
#include "zero_storage.h"
#include "zero_config_cmd.h"
#include "ch32_transport.h"
#include "swo_trace.h"
#include "led_signal.h"
/* Request excludes vendor command byte. Status: 0 success, 1 invalid,
 * 2 busy, 3 storage failure, 4 absent, 5 corrupted. */
uint32_t zero_config_command(const uint8_t *req, uint8_t *resp, bool connected) {
    uint8_t op=req[0];
    resp[0]=1;
    if (op==0 || op==1) {
        zero_settings_t result=zero_settings_default;
        if (op==0) { result=zero_settings_active; resp[0]=0; }
        else {
            enum zero_settings_load_result s=zero_storage_read(&result);
            resp[0]=s==ZERO_SETTINGS_LOADED ? 0 : s==ZERO_SETTINGS_EMPTY ? 4 : 5;
        }
        memcpy(resp+1,&result,sizeof(result));
        return (1u<<16)|(1u+sizeof(result));
    }
    if (op==2) {
        zero_settings_t candidate;
        memcpy(&candidate,req+1,sizeof(candidate));
        if (zero_settings_validate(&candidate)!=ZERO_SETTINGS_OK) resp[0]=1;
        else if (connected || ch32_transport_is_active() || swo_trace_is_active()) resp[0]=2;
        else resp[0]=zero_storage_write(&candidate) ? 0 : 3;
        return ((1u+sizeof(candidate))<<16)|1u;
    }
    if (op==3) {
        resp[0]=(req[2]<=1 && led_signal_test(req[1],req[2])) ? 0 : 1;
        return (3u<<16)|1u;
    }
    return (1u<<16)|1u;
}

#endif
