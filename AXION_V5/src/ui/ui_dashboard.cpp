// AXION V4 — UI: Dashboard

#include <Arduino.h>
#include "axion.h"
#include "sensor_structs.h"
#include "feedback.h"

static float to_kmh(float mps) { return mps * 3.6f; }

void ui_dashboard() {
  oled_clear();
  ui_draw_topbar("DASHBOARD");
  oled_set_font_large();

  // Speed: prefer fused later; currently show GPS and OBD
  char buf[64];
  snprintf(buf, sizeof(buf), "%3.0f", to_kmh(fused_speed_mps()));
  oled_draw_string(10, 20, buf, 15);
  oled_set_font_medium();
  snprintf(buf, sizeof(buf), "GPS:%3.0f OBD:%3.0f", to_kmh(g_gps_data.speed_mps), to_kmh(g_vehicle_data.speed_mps));
  oled_draw_string(120, 22, buf, 12);
  snprintf(buf, sizeof(buf), "RPM:%u THR:%02d%%", g_vehicle_data.rpm, (int)g_vehicle_data.throttle);
  oled_draw_string(120, 34, buf, 11);

  // Status line
  snprintf(buf, sizeof(buf), "GPS:%s(%u/%0.1f) IMU:%s OBD:%s RTC:%s",
           g_gps_data.lock?"OK":"--", g_gps_data.sats, g_gps_data.hdop,
           (millis()-g_imu_data.last_ms<500)?"OK":"--",
           g_vehicle_data.obd_ok?"OK":"--",
           "OK");
  oled_set_font_small();
  oled_draw_string(2, 54, buf, 9);
  bool all_ok = g_gps_data.lock && (millis()-g_imu_data.last_ms<500) && g_vehicle_data.obd_ok;
  feedback_set_dashboard_status(all_ok, !all_ok);

  oled_blit();
}
