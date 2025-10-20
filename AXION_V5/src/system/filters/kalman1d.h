// Simple 1D Kalman filter for scalar fusion (e.g., speed)
#pragma once

struct Kalman1D {
  // x: state estimate, P: covariance, Q: process noise, R: measurement noise
  float x = 0.0f;
  float P = 1.0f;
  float Q = 0.01f;
  float R = 1.0f;
  bool initialized = false;

  void init(float x0, float P0=1.0f) { x = x0; P = P0; initialized = true; }
  void set_noise(float q, float r) { Q = q; R = r; }

  // Predict assumes constant model: x = x ; P = P + Q
  void predict() { P += Q; }

  // Update with measurement z
  void update(float z) {
    if (!initialized) { init(z, 1.0f); }
    float K = P / (P + R);
    x = x + K * (z - x);
    P = (1.0f - K) * P;
  }
};

