// AXION V4 — System state persistence

#include <Arduino.h>
#include "system_state.h"
#include "eeprom_at24c32.h"
#include <esp_system.h>

static constexpr uint16_t EE_ADDR = 0x380; // 16 bytes reserved

void system_state_load(SystemState& s) {
  uint8_t buf[16] = {0};
  if (eeprom_at24c32_read(EE_ADDR, buf, sizeof buf)) {
    memcpy(&s, buf, sizeof(SystemState) <= 16 ? sizeof(SystemState) : 16);
  }
}

void system_state_save(const SystemState& s) {
  uint8_t buf[16] = {0};
  memcpy(buf, &s, sizeof(SystemState) <= 16 ? sizeof(SystemState) : 16);
  eeprom_at24c32_write(EE_ADDR, buf, sizeof buf);
}

void system_state_note_boot() {
  SystemState s{}; system_state_load(s);
  s.boots += 1; s.last_reset_reason = (uint8_t)esp_reset_reason();
  system_state_save(s);
}

void system_state_set_last_error(uint8_t code) {
  SystemState s{}; system_state_load(s); s.last_error = code; system_state_save(s);
}

void system_state_set_last_uptime(uint32_t ms) {
  SystemState s{}; system_state_load(s); s.last_uptime_ms = ms; system_state_save(s);
}
