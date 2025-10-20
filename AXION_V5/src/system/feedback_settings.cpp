// AXION V4 — Feedback preferences persistence

#include <Arduino.h>
#include "feedback_settings.h"
#include "eeprom_at24c32.h"
#include "feedback.h"

static constexpr uint16_t EE_ADDR = 0x3C0;

void feedback_settings_load(FeedbackPrefs& p) {
  uint8_t hdr[4];
  if (!eeprom_at24c32_read(EE_ADDR, hdr, sizeof hdr)) goto defaults;
  if (hdr[0] != 'F' || hdr[1] != 'B') goto defaults;
  if (hdr[2] == 2) {
    if (!eeprom_at24c32_read(EE_ADDR+4, (uint8_t*)&p, sizeof p)) goto defaults;
    return;
  }
  if (hdr[2] == 1) {
    // legacy: no global fields
    memset(&p, 0, sizeof p); p.global_led = true; p.global_buzzer = true;
    // read legacy payload size (arrays only)
    size_t legacy_sz = (size_t) ( (uint8_t)UIMode::Count * 2 ); // bool arrays; but we wrote as bytes
    uint8_t tmp[64] = {0};
    if (eeprom_at24c32_read(EE_ADDR+4, tmp, legacy_sz)) {
      for (uint8_t i=0;i<(uint8_t)UIMode::Count;i++) { p.enable_led[i]=true; p.enable_buzzer[i]=true; }
    }
    return;
  }
defaults:
  p.global_led = true; p.global_buzzer = true;
  for (uint8_t i=0;i<(uint8_t)UIMode::Count;i++){ p.enable_led[i]=true; p.enable_buzzer[i]=true; }
}

void feedback_settings_save(const FeedbackPrefs& p) {
  uint8_t hdr[4]={'F','B',2,0};
  eeprom_at24c32_write(EE_ADDR, hdr, sizeof hdr);
  eeprom_at24c32_write(EE_ADDR+4, (const uint8_t*)&p, sizeof p);
}

void feedback_settings_apply(const FeedbackPrefs& p) {
  feedback_set_global(p.global_led, p.global_buzzer);
  for (uint8_t i=0;i<(uint8_t)UIMode::Count;i++) {
    feedback_set_enable((UIMode)i, p.enable_led[i], p.enable_buzzer[i]);
  }
}
