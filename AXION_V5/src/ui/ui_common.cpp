// AXION V4 — UI common helpers (top bar, icons)

#include <Arduino.h>
#include "axion.h"
#include "sensor_structs.h"

static uint8_t gray_active = 14;   // very light (like 'red')
static uint8_t gray_ok     = 13;   // light (like 'green')
static uint8_t gray_mid    = 9;    // medium (like 'yellow')
static uint8_t gray_inact  = 5;    // dark
static char g_toast[64];
static uint32_t g_toast_until = 0;

void ui_draw_topbar(const char* mode_name) {
  // Bar background
  oled_fill_rect(0, 0, 256, 10, 3);
  // Mode name
  oled_set_font_small();
  oled_draw_string(2, 1, mode_name ? mode_name : "", 15);

  // Sensor icons (small squares)
  int x = 200; int y = 2; int s = 6; int gap = 4;
  // GPS
  uint8_t g = (!g_gps_data.lock ? gray_inact : (g_gps_data.hdop < 1.5f ? gray_ok : gray_mid));
  oled_fill_rect(x, y, s, s, g); x += s + gap;
  // OBD
  g = (g_vehicle_data.obd_ok ? gray_ok : gray_inact);
  oled_fill_rect(x, y, s, s, g); x += s + gap;
  // IMU
  g = ((millis()-g_imu_data.last_ms<500) ? gray_ok : gray_inact);
  oled_fill_rect(x, y, s, s, g); x += s + gap;
  // RTC (assumed OK if rtc_get_unix()!=0)
  g = (rtc_get_unix()!=0 ? gray_ok : gray_inact);
  oled_fill_rect(x, y, s, s, g);
}

void toast_show(const char* msg, uint32_t duration_ms) {
  if (!msg) return;
  strncpy(g_toast, msg, sizeof(g_toast)-1);
  g_toast[sizeof(g_toast)-1] = 0;
  g_toast_until = millis() + duration_ms;
}

void toast_draw_if_any() {
  if (g_toast_until == 0) return;
  if ((int32_t)(millis() - g_toast_until) > 0) { g_toast_until = 0; return; }
  // Draw a top toast bar
  oled_fill_rect(0, 0, 256, 10, 6);
  oled_set_font_small();
  oled_draw_string(2, 1, g_toast, 15);
}
