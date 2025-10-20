// AXION V4 — Drift estimator (theta_drift and score)

#include <Arduino.h>
#include "sensor_structs.h"

float g_drift_theta_deg = 0.0f;
float g_drift_score = 0.0f;

static float wrap180(float a) { while (a>180.0f) a-=360.0f; while (a<-180.0f) a+=360.0f; return a; }

void drift_update() {
  bool valid = (g_gps_data.speed_mps*3.6f > 20.0f && g_gps_data.hdop > 0 && g_gps_data.hdop < 2.0f);
  if (valid) {
    g_drift_theta_deg = wrap180(g_imu_data.yaw - g_gps_data.heading_deg);
    g_drift_score = fabsf(g_drift_theta_deg) * 0.5f + fabsf(g_imu_data.ay) * 10.0f + (g_gps_data.speed_mps*3.6f) * 0.05f;
  }
}

