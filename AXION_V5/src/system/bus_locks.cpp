// AXION V4 — Bus locks (I2C/SPI) for FreeRTOS tasks

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static SemaphoreHandle_t g_i2c = nullptr;
static SemaphoreHandle_t g_spi = nullptr;

void bus_locks_init() {
  if (!g_i2c) g_i2c = xSemaphoreCreateMutex();
  if (!g_spi) g_spi = xSemaphoreCreateMutex();
}

void bus_i2c_lock()   { if (g_i2c) xSemaphoreTake(g_i2c, portMAX_DELAY); }
void bus_i2c_unlock() { if (g_i2c) xSemaphoreGive(g_i2c); }
void bus_spi_lock()   { if (g_spi) xSemaphoreTake(g_spi, portMAX_DELAY); }
void bus_spi_unlock() { if (g_spi) xSemaphoreGive(g_spi); }

