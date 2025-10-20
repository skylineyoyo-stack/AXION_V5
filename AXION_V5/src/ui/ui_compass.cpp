// AXION V4 — UI: Compass

#include <Arduino.h>
#include "axion.h"
#include "sensor_structs.h"

void ui_compass() {
  float yaw = g_imu_data.yaw;
  float psi = (g_gps_data.speed_mps > 2.0f) ? g_gps_data.heading_deg : yaw;
  oled_clear();
  oled_draw_text(2, 2, "COMPASS");
  char buf[64];
  snprintf(buf, sizeof(buf), "IMU: %0.1f deg", yaw);
  oled_draw_text(2, 14, buf);
  snprintf(buf, sizeof(buf), "GPS: %0.1f deg", g_gps_data.heading_deg);
  oled_draw_text(2, 24, buf);
  snprintf(buf, sizeof(buf), "HDG: %0.1f deg", psi);
  oled_draw_text(2, 36, buf);
  oled_blit();
}
