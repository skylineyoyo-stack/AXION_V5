// AXION V4 — Error logger (rotating buffer + EEPROM persist)
#pragma once
#include <Arduino.h>

struct ErrorEntry {
  uint32_t epoch;   // RTC epoch seconds
  uint8_t  code;    // error/warn code
  uint8_t  src;     // hashed source id
  int32_t  data;    // optional data
};

// Initialize error logger
void error_log_init();

// Add an entry (source hashed internally to 0..255)
void error_log(uint8_t code, const char* source, int32_t data);

// Dump last N entries to Serial
void error_log_dump(uint16_t last_n = 20);

// Persist last K entries to EEPROM (@0x300 area)
bool error_log_persist(uint16_t max_to_save = 128);

// Count current entries
uint16_t error_log_count();
// Copy last N entries into provided buffer (newest last). Returns count written.
uint16_t error_log_copy_last(ErrorEntry* out, uint16_t max);

// Codes common
enum ErrorCodes : uint8_t {
  ERR_GPS_LOST = 1,
  ERR_IMU_FAIL = 2,
  ERR_OBD_TIMEOUT = 3,
  ERR_SD_FLUSH = 4,
  ERR_HEAP_LOW = 5,
  ERR_REBOOT_CAUSE = 6,
  ERR_SD_WRITE_FAIL = 7,
};
