// AXION V4 — AT24C32 EEPROM minimal I2C driver
#pragma once

#include <Arduino.h>

bool eeprom_at24c32_write(uint16_t addr, const uint8_t* data, size_t len);
bool eeprom_at24c32_read(uint16_t addr, uint8_t* data, size_t len);

