#ifndef ZERO_STORAGE_H
#define ZERO_STORAGE_H
#include "zero_settings.h"
void zero_storage_init(void);
enum zero_settings_load_result zero_storage_read(zero_settings_t *out);
bool zero_storage_write(const zero_settings_t *settings);
#endif
