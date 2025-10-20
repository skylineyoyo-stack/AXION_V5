// AXION V4 — AT24C32 EEPROM minimal driver

#include <Arduino.h>
#include <Wire.h>
#include "hardware/pins.h"
#include "eeprom_at24c32.h"
#include "axion.h"

static constexpr uint8_t DEV = I2C_ADDR_EEPROM; // 0x50
static constexpr size_t PAGE = 32; // 32-byte pages

bool eeprom_at24c32_write(uint16_t addr, const uint8_t* data, size_t len) {
  size_t off = 0;
  bus_i2c_lock();
  while (off < len) {
    size_t page_off = addr % PAGE;
    size_t chunk = min(len - off, PAGE - page_off);
    Wire.beginTransmission(DEV);
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr & 0xFF));
    Wire.write(data + off, chunk);
    if (Wire.endTransmission() != 0) return false;
    delay(5); // write cycle
    addr += chunk; off += chunk;
  }
  bus_i2c_unlock();
  return true;
}

bool eeprom_at24c32_read(uint16_t addr, uint8_t* data, size_t len) {
  bus_i2c_lock();
  Wire.beginTransmission(DEV);
  Wire.write((uint8_t)(addr >> 8));
  Wire.write((uint8_t)(addr & 0xFF));
  if (Wire.endTransmission(false) != 0) return false;
  size_t got = Wire.requestFrom((int)DEV, (int)len);
  if (got != len) return false;
  for (size_t i=0;i<len;++i) data[i] = Wire.read();
  bus_i2c_unlock();
  return true;
}
