// AXION V4 — UI: G-Forces

#include <Arduino.h>
#include "axion.h"
#include "sensor_structs.h"

void ui_gforces() {
  oled_clear();
  ui_draw_topbar("G-FORCES");
  oled_set_font_medium();
  char buf[64];
  snprintf(buf, sizeof(buf), "Gx:%+0.2f Gy:%+0.2f", g_imu_data.ax, g_imu_data.ay);
  oled_draw_string(2, 14, buf, 12);
  snprintf(buf, sizeof(buf), "v_gps:%0.1f km/h", g_gps_data.speed_mps*3.6f);
  oled_draw_string(2, 24, buf, 10);
  // Simple cross and marker with LGFX
  int cx = 128, cy = 32;
  oled_fill_rect(cx-20, cy, 41, 1, 15);
  oled_fill_rect(cx, cy-10, 1, 21, 15);
  int px = cx + (int)(g_imu_data.ax * (20.0f/1.5f));
  int py = cy - (int)(g_imu_data.ay * (10.0f/1.5f));
  oled_fill_rect(px-1, py-1, 3, 3, 14);
  oled_blit();
}
