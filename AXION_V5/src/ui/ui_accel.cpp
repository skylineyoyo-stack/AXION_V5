// AXION V4 — UI: Accel & Brake (basic 0-100)

#include <Arduino.h>
#include "axion.h"
#include "feedback.h"
#include "sensor_structs.h"

static bool running = false;
static uint32_t t_start = 0;
static uint32_t t_100 = 0;

void ui_accel() {
  float v = g_gps_data.speed_mps * 3.6f;
  float gx = g_imu_data.ax;

  if (!running) {
    if (v > 1.0f && gx > 0.3f) { running = true; t_start = millis(); t_100 = 0; }
  } else {
    if (t_100 == 0 && v >= 100.0f) { t_100 = millis(); feedback_emit(UIMode::Accel, FbEvent::FINISH); }
  }

  oled_clear();
  oled_draw_text(2, 2, "ACCEL TEST 0-100");
  char buf[64];
  snprintf(buf, sizeof(buf), "v=%.1f km/h  Gx=%.2f", v, gx);
  oled_draw_text(2, 14, buf);
  if (!running) {
    oled_draw_text(2, 26, "Ready: press throttle (>0.3G)");
  } else {
    uint32_t dt = millis() - t_start;
    snprintf(buf, sizeof(buf), "t=%.3fs", dt / 1000.0f);
    oled_draw_text(2, 26, buf);
    if (t_100) {
      float t = (t_100 - t_start) / 1000.0f;
      snprintf(buf, sizeof(buf), "0-100: %.3fs", t);
      oled_draw_text(2, 38, buf);
    }
  }
  oled_blit();
}

bool accel_run_active() { return running; }
