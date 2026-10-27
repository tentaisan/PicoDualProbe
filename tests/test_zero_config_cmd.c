#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "zero_settings.h"
#include "zero_config_cmd.h"
static zero_settings_t saved;
static bool exists, busy, swo, fail;
enum zero_settings_load_result zero_storage_read(zero_settings_t *s) {
 if (!exists) return ZERO_SETTINGS_EMPTY;
 *s=saved; return ZERO_SETTINGS_LOADED;
}
bool zero_storage_write(const zero_settings_t *s) { if (fail) return false; saved=*s; exists=true; return true; }
bool ch32_transport_is_active(void) { return busy; }
bool swo_trace_is_active(void) { return swo; }
bool led_signal_test(uint8_t role,bool on) { return role<5 && on<=1; }
int main(void) {
 uint8_t req[32]={0}, out[32]={0}; zero_settings_t s=zero_settings_default;
 zero_settings_active=s;
 assert(zero_config_command(req,out,false)==((1u<<16)|18)); assert(out[0]==0 && out[1]==1);
 req[0]=1; zero_config_command(req,out,false); assert(out[0]==4);
 req[0]=2; memcpy(req+1,&s,sizeof(s));
 zero_config_command(req,out,true); assert(out[0]==2 && !exists);
 busy=true; zero_config_command(req,out,false); assert(out[0]==2); busy=false;
 swo=true; zero_config_command(req,out,false); assert(out[0]==2); swo=false;
 fail=true; zero_config_command(req,out,false); assert(out[0]==3); fail=false;
 assert(zero_config_command(req,out,false)==((18u<<16)|1)); assert(!out[0] && exists);
 req[0]=1; zero_config_command(req,out,false); assert(!out[0] && out[1]==1);
 s.swdio=4; req[0]=2; memcpy(req+1,&s,sizeof(s)); zero_config_command(req,out,false); assert(out[0]==1);
 req[0]=3; req[1]=2; req[2]=1; zero_config_command(req,out,false); assert(!out[0]);
 puts("Zero settings command tests passed");
}
