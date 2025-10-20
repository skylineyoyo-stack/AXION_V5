// AXION V4 — Buzzer

#include <Arduino.h>
#include "hardware/pins.h"

void buzzer_begin() {
  ledcSetup(LEDC_CH_BUZZ, 2000, 8);
  ledcAttachPin(PIN_BUZZER, LEDC_CH_BUZZ);
  ledcWrite(LEDC_CH_BUZZ, 0);
}

void buzzer_beep(uint16_t freq, uint16_t ms, uint8_t duty) {
  ledcWriteTone(LEDC_CH_BUZZ, freq);
  ledcWrite(LEDC_CH_BUZZ, duty);
  delay(ms);
  ledcWrite(LEDC_CH_BUZZ, 0);
}
