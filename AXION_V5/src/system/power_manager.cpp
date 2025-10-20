// AXION V4 — Power manager

#include <Arduino.h>
#include "hardware/pins.h"
#include "axion.h"
#include <esp_sleep.h>
#include "error_logger.h"
#include "system_state.h"

static uint32_t last_activity_ms = 0;
static bool want_deep = false;

void power_note_activity() { last_activity_ms = millis(); }

void power_request_deep_sleep() { want_deep = true; }

static void enter_light_sleep() {
  // Turn OLED off
  system_state_set_last_uptime(millis());
  logger_flush();
  error_log_persist();
  oled_sleep(true);
  // Wake on IMU INT or RTC SQW
  esp_sleep_enable_ext1_wakeup((1ULL<<PIN_INT_MPU) | (1ULL<<PIN_SQW_RTC), ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_light_sleep_start();
  // On wake
  oled_sleep(false);
}

static void enter_deep_sleep() {
  system_state_set_last_uptime(millis());
  logger_flush();
  error_log_persist();
  oled_sleep(true);
  esp_sleep_enable_ext1_wakeup((1ULL<<PIN_INT_MPU) | (1ULL<<PIN_SQW_RTC), ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_deep_sleep_start();
}

void power_init() {
  last_activity_ms = millis();
}

void power_tick() {
  if (want_deep) { enter_deep_sleep(); }
  // Inactivity 5 min → light sleep
  if ((millis() - last_activity_ms) > 5UL*60UL*1000UL) {
    enter_light_sleep();
    last_activity_ms = millis();
  }
}
