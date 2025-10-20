// AXION V4 — Persistent system state
#pragma once
#include <Arduino.h>

struct SystemState {
  uint32_t boots = 0;
  uint32_t last_uptime_ms = 0;
  uint8_t  last_error = 0;
  uint8_t  last_reset_reason = 0;
  uint16_t reserved = 0;
};

void system_state_load(SystemState& s);
void system_state_save(const SystemState& s);
void system_state_note_boot();
void system_state_set_last_error(uint8_t code);

