// AXION V4 — Sensor data structures
#pragma once

#include <Arduino.h>

struct GPSData {
  double lat = 0.0;
  double lon = 0.0;
  float alt = 0.0f;
  float speed_mps = 0.0f;
  float heading_deg = 0.0f;
  uint8_t sats = 0;
  float hdop = 0.0f;
  uint32_t last_fix_ms = 0;
  bool lock = false;
};

struct IMUData {
  float ax = 0.0f, ay = 0.0f, az = 0.0f;
  float gx = 0.0f, gy = 0.0f, gz = 0.0f;
  float mx = 0.0f, my = 0.0f, mz = 0.0f;
  float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
  uint32_t last_ms = 0;
};

struct VehicleData {
  uint16_t rpm = 0;
  float throttle = 0.0f;
  float coolant_temp = 0.0f;
  uint32_t last_ms = 0;
  float speed_mps = 0.0f; // OBD vehicle speed if available
  bool obd_ok = false;
};

// Global sensor states
extern GPSData g_gps_data;
extern IMUData g_imu_data;
extern VehicleData g_vehicle_data;

// Drift estimation shared fields
extern float g_drift_theta_deg;
extern float g_drift_score;

// Drag racing
struct drag_run_t {
  bool armed = false;
  bool countdown = false;
  bool running = false;
  bool finished = false;
  uint32_t t_start = 0;
  uint32_t t_last = 0;
  float dist_m = 0.0f;
  float v_end_mps = 0.0f;
  float gmax = 0.0f;
  // Splits timestamps (ms, 0 if not yet)
  uint32_t t_60ft = 0;
  uint32_t t_330ft = 0;
  uint32_t t_660ft = 0;
  uint32_t t_final = 0;
};
