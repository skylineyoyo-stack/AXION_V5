// AXION V4 — UI: Drift (theta_drift + score placeholder)

#include <Arduino.h>
#include "axion.h"
#include <math.h>
#include "sensor_structs.h"

static float wrap180(float a) {
  while (a > 180.0f) a -= 360.0f;
  while (a < -180.0f) a += 360.0f;
  return a;
}

void ui_drift() {
  float v = g_gps_data.speed_mps * 3.6f;
  bool valid = (v > 20.0f && g_gps_data.hdop > 0 && g_gps_data.hdop < 2.0f);
  float theta = valid ? wrap180(g_imu_data.yaw - g_gps_data.heading_deg) : 0.0f;
  float score = valid ? (fabsf(theta) * 0.5f + fabsf(g_imu_data.ay) * 10.0f + v * 0.05f) : 0.0f;
  g_drift_theta_deg = theta; g_drift_score = score;

  // Simple G-trace ring buffer
  static int head = 0; static int count = 0; static int16_t pts[128][2];
  int gx = (int)(g_imu_data.ax * 20.0f); if (gx<-20) gx=-20; if (gx>20) gx=20;
  int gy = (int)(g_imu_data.ay * 20.0f); if (gy<-20) gy=-20; if (gy>20) gy=20;
  pts[head][0] = gx; pts[head][1] = gy; head = (head+1) & 127; if (count<128) count++;

  oled_clear();
  oled_draw_text(2, 2, "DRIFT");
  char buf[64];
  snprintf(buf, sizeof(buf), "θ=%+0.1f  v=%.1f  s=%.1f", theta, v, score);
  oled_draw_text(2, 14, buf);

  // Draw circle and trace
  int cx=200, cy=32, r=20;
  for (int a=0;a<360;a+=5) {
    int x=cx+(int)(r*cos(a*0.0174533)); int y=cy+(int)(r*sin(a*0.0174533)); oled_draw_pixel(x,y,8);
  }
  for (int i=0;i<count;i++) {
    int idx = (head - 1 - i) & 127;
    int x=cx+pts[idx][0]; int y=cy-pts[idx][1];
    uint8_t g = (uint8_t)(15 - (i/16)); if (g<4) g=4;
    oled_draw_pixel(x,y,g);
  }

#ifdef USE_SD
  // SELECT toggles drift session logging
  static bool sel_down=false; static uint32_t t0=0;
  if (btn_select() && !sel_down) { sel_down=true; t0=millis(); }
  if (!btn_select() && sel_down) { sel_down=false; if (millis()-t0<500) { if (!session_drift_active()) session_start_drift(); else session_stop_drift(); } }
  oled_draw_text(2, 54, session_drift_active()?"Session: REC":"Session: idle");
#endif
  oled_blit();
}
