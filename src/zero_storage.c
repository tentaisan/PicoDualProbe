/* Last two flash sectors hold alternating, CRC-protected settings records. */
#include "probe_config.h"
#ifdef DEBUG_ON_ZERO
#include <string.h>
#include "hardware/flash.h"
#include "hardware/address_mapped.h"
#include "pico/flash.h"
#include "zero_storage.h"
#define SETTINGS_OFFSET (PICO_FLASH_SIZE_BYTES - 2u * FLASH_SECTOR_SIZE)
extern char __flash_binary_end;
static uint8_t page[FLASH_PAGE_SIZE];
static uint32_t write_offset;
static const uint8_t *slot(unsigned i) { return (const uint8_t *)(XIP_BASE+SETTINGS_OFFSET+i*FLASH_SECTOR_SIZE); }
void zero_storage_init(void) {
    zero_settings_active=zero_settings_default;
    (void)zero_storage_read(&zero_settings_active);
}
enum zero_settings_load_result zero_storage_read(zero_settings_t *out) {
    int selected=zero_settings_select(slot(0),slot(1),out,NULL);
    return selected>=0 ? ZERO_SETTINGS_LOADED : selected==-1 ? ZERO_SETTINGS_EMPTY : ZERO_SETTINGS_CORRUPT;
}
static void write_sector(void *arg) {
    (void)arg;
    flash_range_erase(write_offset,FLASH_SECTOR_SIZE);
    flash_range_program(write_offset,page,FLASH_PAGE_SIZE);
}
bool zero_storage_write(const zero_settings_t *settings) {
    if (zero_settings_validate(settings)!=ZERO_SETTINGS_OK ||
        (uintptr_t)&__flash_binary_end > XIP_BASE+SETTINGS_OFFSET) return false;
    zero_settings_t old;
    uint32_t sequence=0;
    int current=zero_settings_select(slot(0),slot(1),&old,&sequence);
    unsigned next=current==0 ? 1u : 0u;
    write_offset=SETTINGS_OFFSET+next*FLASH_SECTOR_SIZE;
    memset(page,0xff,sizeof(page));
    zero_settings_encode_sequence(settings,sequence+1u,page);
    if (flash_safe_execute(write_sector,NULL,1000)!=PICO_OK) return false;
    zero_settings_t check;
    return zero_storage_read(&check)==ZERO_SETTINGS_LOADED && !memcmp(&check,settings,sizeof(check));
}
#endif
