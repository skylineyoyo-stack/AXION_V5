// AXION V4 — Speed fusion (GPS + OBD) via simple 1D Kalman

#include "sensor_structs.h"
#include "system/filters/kalman1d.h"

static Kalman1D kf;
static bool inited = false;
static uint8_t conf = 0;

void speed_fusion_tick() {
  if (!inited) { kf.init(0.0f, 10.0f); kf.set_noise(0.05f, 1.0f); inited = true; }
  // Predict
  kf.predict();

  // Update with GPS (if available)
  if (g_gps_data.lock) {
    float Rgps = 1.0f + (g_gps_data.hdop > 0 ? g_gps_data.hdop : 1.0f); // rough
    float oldR = kf.R; kf.R = Rgps; kf.update(g_gps_data.speed_mps); kf.R = oldR;
  }

  // Update with OBD (if available)
  if (g_vehicle_data.obd_ok) {
    float oldR = kf.R; kf.R = 1.5f; kf.update(g_vehicle_data.speed_mps); kf.R = oldR;
  }

  // Confidence heuristic
  uint8_t c = 0;
  if (g_gps_data.lock) {
    if (g_gps_data.hdop < 0.8f) c += 50; else if (g_gps_data.hdop < 1.5f) c += 35; else c += 20;
  }
  if (g_vehicle_data.obd_ok) c += 30;
  // Clamp 0..100
  conf = (c > 100) ? 100 : c;
}

float fused_speed_mps() { return kf.x; }
uint8_t speed_confidence_pct() { return conf; }

