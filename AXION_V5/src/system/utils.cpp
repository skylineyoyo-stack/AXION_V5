// AXION V4 — Utilities

#include <Arduino.h>
#include <Wire.h>

void i2c_scan() {
  Serial.println("I2C scan:");
  for (uint8_t a = 1; a < 127; ++a) {
    Wire.beginTransmission(a);
    uint8_t e = Wire.endTransmission();
    if (e == 0) {
      Serial.printf(" - 0x%02X\n", a);
    }
  }
}

