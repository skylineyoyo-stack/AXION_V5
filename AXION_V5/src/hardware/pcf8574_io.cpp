// AXION V4 — PCF8574 I/O (buttons, LEDs, OLED CS/RESET)

#include <Arduino.h>
#include <Wire.h>
#include "axion.h"
#include "hardware/pins.h"

static uint8_t g_pcf_state = 0xFF; // 1=released/input HIGH, 0=drive LOW

void pcf_begin() {
  g_pcf_state = 0xFF; // inputs P0..P2 released, outputs high
  bus_i2c_lock();
  Wire.beginTransmission(I2C_ADDR_PCF8574);
  Wire.write(g_pcf_state);
  Wire.endTransmission();
  bus_i2c_unlock();
  delay(5);
}

void pcf_set_bit(uint8_t bit, bool high) {
  if (high) g_pcf_state |= (1u << bit);
  else      g_pcf_state &= ~(1u << bit);
  bus_i2c_lock();
  Wire.beginTransmission(I2C_ADDR_PCF8574);
  Wire.write(g_pcf_state);
  Wire.endTransmission();
  bus_i2c_unlock();
}

bool pcf_read_bit(uint8_t bit) {
  bus_i2c_lock();
  Wire.requestFrom((int)I2C_ADDR_PCF8574, 1);
  if (Wire.available()) {
    g_pcf_state = Wire.read();
  }
  bus_i2c_unlock();
  return (g_pcf_state >> bit) & 1u;
}

bool btn_up()     { return !pcf_read_bit(PCF_BIT_BTN_UP); }
bool btn_down()   { return !pcf_read_bit(PCF_BIT_BTN_DOWN); }
bool btn_select() { return !pcf_read_bit(PCF_BIT_BTN_SELECT); }

void led_set(bool red_on, bool green_on) {
  // LEDs are LOW=ON
  pcf_set_bit(PCF_BIT_LED_RED,   !red_on);
  pcf_set_bit(PCF_BIT_LED_GREEN, !green_on);
}

void oled_cs(bool active) {
  // Active LOW via PCF8574 P6
  pcf_set_bit(PCF_BIT_OLED_CS, !active);
}

void oled_reset_pulse() {
  pcf_set_bit(PCF_BIT_OLED_RST, false);
  delay(20);
  pcf_set_bit(PCF_BIT_OLED_RST, true);
  delay(20);
}
