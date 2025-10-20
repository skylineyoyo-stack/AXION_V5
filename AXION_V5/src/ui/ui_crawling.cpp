// AXION V4 — UI: Crawling (basic)

#include <Arduino.h>
#include "axion.h"
#include "sensor_structs.h"

void ui_crawling() {
  oled_clear();
  oled_draw_text(2, 2, "CRAWLING");
  char buf[64];
  snprintf(buf, sizeof(buf), "Alt: %.1fm", g_gps_data.alt);
  oled_draw_text(2, 14, buf);
  snprintf(buf, sizeof(buf), "Pitch: %+0.1f Roll: %+0.1f", g_imu_data.pitch, g_imu_data.roll);
  oled_draw_text(2, 24, buf);
  oled_blit();
}
