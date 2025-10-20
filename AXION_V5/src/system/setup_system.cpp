// AXION V4 — Setup & high-level loop

#include <Arduino.h>
#include <Wire.h>
#include "axion.h"
#include "system_config.h"
#include "ui_definitions.h"
#include "hardware/pins.h"
#include "sensor_structs.h"
#include "error_logger.h"
#include "system_state.h"
#include "feedback_settings.h"
#include "feedback.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

volatile bool g_flag_pps = false;
volatile bool g_flag_sqw = false;
volatile bool g_flag_int_imu  = false;
volatile uint32_t g_ts_pps_us = 0;
volatile uint32_t g_ts_sqw_us = 0;
UIMode g_mode = UIMode::Dashboard;

static uint32_t t_ms_ui = 0;
static uint32_t t_ms_log = 0;

// ISRs
void IRAM_ATTR isr_pps() { g_flag_pps = true; g_ts_pps_us = (uint32_t)micros(); }
void IRAM_ATTR isr_sqw() { g_flag_sqw = true; g_ts_sqw_us = (uint32_t)micros(); }
void IRAM_ATTR isr_imu() { g_flag_int_imu  = true; }

void axion_setup() {
  Serial.begin(115200);
  delay(80);
  Serial.printf("\nAXION V%u.%u boot...\n", AXION_MAJOR, AXION_MINOR);
  bus_locks_init();
  error_log_init();
  system_state_note_boot();

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  delay(10);
  i2c_scan();

  pcf_begin();
  led_set(false, true); // green during boot

  buzzer_begin();
  buzzer_beep(2400, 40, 64);

  oled_begin();
  Serial.printf("Free heap at boot: %u bytes\n", (unsigned)ESP.getFreeHeap());
#ifndef NO_BOOT_SPLASH
  // Boot screen (2s)
  oled_clear();
  oled_set_font_large();
  oled_draw_string(80, 24, "AXION", 15);
  oled_blit();
  delay(2000);
#endif

  gps_begin();
  rtc_begin();
  imu_begin();
  obd2_begin();
  // Load saved lap track if available
  lap_load_from_eeprom();

  // Interrupts
  pinMode(PIN_GPS_PPS, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_GPS_PPS), isr_pps, RISING);
  pinMode(PIN_SQW_RTC, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_SQW_RTC), isr_sqw, RISING);
  pinMode(PIN_INT_MPU, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_INT_MPU), isr_imu, RISING);

  logger_init();
  power_init();
  menu_init();
  feedback_init();

  led_set(false, false);

  t_ms_ui  = millis();
  t_ms_log = millis();
  Serial.println("Boot complete.");

  // Create dual-core tasks (UI on core 1, sensors on core 0)
  auto ui_task = [](void*){
    const TickType_t dt = pdMS_TO_TICKS(1000 / UI_FPS);
    for(;;){ menu_update(); ui_render_current(); feedback_tick(); vTaskDelay(dt); }
  };
  auto sensor_task = [](void*){
    TickType_t last = xTaskGetTickCount();
    uint32_t boot_ms = millis(); bool notified=false;
    for(;;){
      gps_tick();
      imu_tick();
      obd2_tick();
      speed_fusion_tick();
      drift_update();
      power_tick();
      logger_tick();
      // Startup sensor toast after 5s
      if (!notified && (millis() - boot_ms > 5000)) {
        bool any=false; uint32_t now=millis();
        if (!g_gps_data.lock || (now - g_gps_data.last_fix_ms > 2000)) { error_log(ERR_GPS_LOST, "GPS", 0); any=true; }
        if (now - g_imu_data.last_ms > 2000) { error_log(ERR_IMU_FAIL, "IMU", 0); any=true; }
        if (!obd2_is_connected()) { error_log(ERR_OBD_TIMEOUT, "OBD", 0); any=true; }
        uint32_t mu = micros(); if (g_ts_sqw_us == 0 || (mu - g_ts_sqw_us > 2000000)) { error_log(ERR_REBOOT_CAUSE, "RTC", 0); any=true; }
        if (any) { toast_show("Sensor offline  log saved", 2500); }
        notified = true;
      }
      vTaskDelayUntil(&last, pdMS_TO_TICKS(10)); // ~100 Hz service
    }
  };
  xTaskCreatePinnedToCore(ui_task, "UI", 4096, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(sensor_task, "Sensors", 4096, nullptr, 1, nullptr, 0);
}

void axion_loop() {
  // Tasks handle everything
  vTaskDelay(pdMS_TO_TICKS(50));
}

