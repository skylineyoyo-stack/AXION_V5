// AXION V4 — LED/Buzzer feedback router
#pragma once
#include <Arduino.h>
#include "ui_definitions.h"

enum class FbEvent : uint8_t {
  NONE=0,
  OK,
  WARN,
  ERROR,
  RECORD,
  GO,
  FINISH,
  CONNECT,
  DISCONNECT,
  CROSSING,
  BEST,
  DRIFT_ACTIVE,
  DRIFT_SCORE_HIGH,
  DEEP_SLEEP
};

void feedback_init();
void feedback_tick();
void feedback_set_mode(UIMode m);
void feedback_emit(UIMode m, FbEvent e);
void feedback_set_enable(UIMode m, bool led, bool buzzer);
void feedback_set_dashboard_status(bool all_ok, bool sensor_lost_long);
void feedback_set_global(bool led, bool buzzer);
