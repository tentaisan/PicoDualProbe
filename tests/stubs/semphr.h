#pragma once
typedef void *SemaphoreHandle_t;
SemaphoreHandle_t xSemaphoreCreateMutex(void);
void xSemaphoreGive(SemaphoreHandle_t);
void xSemaphoreTake(SemaphoreHandle_t, unsigned);
void vSemaphoreDelete(SemaphoreHandle_t);
