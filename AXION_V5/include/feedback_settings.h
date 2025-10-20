// AXION V4 — Feedback preferences (EEPROM)
#pragma once
#include <Arduino.h>
#include "ui_definitions.h"

struct FeedbackPrefs {
  bool global_led;
  bool global_buzzer;
  bool enable_led[ (uint8_t)UIMode::Count ];
  bool enable_buzzer[ (uint8_t)UIMode::Count ];
};

void feedback_settings_load(FeedbackPrefs& p);
void feedback_settings_save(const FeedbackPrefs& p);
void feedback_settings_apply(const FeedbackPrefs& p);
