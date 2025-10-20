// AXION V4 — Error logger implementation

#include <Arduino.h>
#include "error_logger.h"
#include "eeprom_at24c32.h"

static constexpr uint16_t EE_BASE = 0x300; // EEPROM base
static constexpr uint16_t EE_MAX_BYTES = 1024 * 2; // up to 2 KB reserved

static ErrorEntry g_buf[256];
static uint16_t g_head = 0; // next write index
static uint16_t g_count = 0;

static uint8_t hash_src(const char* s) {
  uint8_t h = 0; if (!s) return 0; while (*s) { h = (uint8_t)(h * 131u + (uint8_t)*s++); } return h; }

void error_log_init() { g_head = 0; g_count = 0; }

void error_log(uint8_t code, const char* source, int32_t data) {
  ErrorEntry e{}; e.epoch = (uint32_t)time(nullptr); e.code = code; e.src = hash_src(source); e.data = data;
  g_buf[g_head] = e; g_head = (uint16_t)((g_head + 1) & 0xFF); if (g_count < 256) g_count++;
}

void error_log_dump(uint16_t last_n) {
  if (last_n == 0 || last_n > g_count) last_n = g_count;
  Serial.printf("\n-- Error Log (last %u) --\n", last_n);
  for (int i = (int)last_n - 1; i >= 0; --i) {
    uint16_t idx = (uint16_t)((g_head + 256 - 1 - i) & 0xFF);
    const ErrorEntry& e = g_buf[idx];
    Serial.printf("t=%lu code=%u src=%u data=%ld\n", (unsigned long)e.epoch, (unsigned)e.code, (unsigned)e.src, (long)e.data);
  }
}

bool error_log_persist(uint16_t max_to_save) {
  if (max_to_save == 0) return true;
  if (max_to_save > 128) max_to_save = 128; // keep under 2KB
  uint16_t n = (g_count < max_to_save) ? g_count : max_to_save;
  // write header [count:2]
  uint8_t hdr[2] = { (uint8_t)(n & 0xFF), (uint8_t)((n>>8)&0xFF) };
  if (!eeprom_at24c32_write(EE_BASE, hdr, sizeof hdr)) return false;
  // write entries compact: 12 bytes each: epoch(4), code(1), src(1), data(4), pad(2)
  uint8_t rec[12];
  uint16_t off = EE_BASE + 2;
  for (uint16_t k = 0; k < n; ++k) {
    uint16_t idx = (uint16_t)((g_head + 256 - n + k) & 0xFF);
    const ErrorEntry& e = g_buf[idx];
    rec[0] = (uint8_t)(e.epoch & 0xFF);
    rec[1] = (uint8_t)((e.epoch>>8)&0xFF);
    rec[2] = (uint8_t)((e.epoch>>16)&0xFF);
    rec[3] = (uint8_t)((e.epoch>>24)&0xFF);
    rec[4] = e.code;
    rec[5] = e.src;
    rec[6] = (uint8_t)(e.data & 0xFF);
    rec[7] = (uint8_t)((e.data>>8)&0xFF);
    rec[8] = (uint8_t)((e.data>>16)&0xFF);
    rec[9] = (uint8_t)((e.data>>24)&0xFF);
    rec[10] = rec[11] = 0;
    if (!eeprom_at24c32_write(off, rec, sizeof rec)) return false;
    off += sizeof rec;
    if (off - EE_BASE > EE_MAX_BYTES) break;
  }
  return true;
}

uint16_t error_log_count() { return g_count; }

uint16_t error_log_copy_last(ErrorEntry* out, uint16_t max) {
  if (!out || max == 0) return 0;
  uint16_t n = (g_count < max) ? g_count : max;
  for (uint16_t k = 0; k < n; ++k) {
    uint16_t idx = (uint16_t)((g_head + 256 - n + k) & 0xFF);
    out[k] = g_buf[idx];
  }
  return n;
}
