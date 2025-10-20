// AXION V4 — UI enumerations
#pragma once

#include <Arduino.h>

enum class UIMode : uint8_t {
  Dashboard = 0,
  GForces,
  Accel,
  Drag,
  Lap,
  Crawling,
  Drift,
  Compass,
  Diagnostics,
  Settings,
  Bench,
  Count
};
