// AXION V4 — RTC DS3231
// - Configure SQW 1 Hz on startup
// - Sync from GPS epoch (UNIX) to DS3231 registers
// - Read DS3231 datetime and return UNIX epoch

#include <Arduino.h>
#include <Wire.h>
#include "hardware/pins.h"
#include "axion.h"

// ---------- Helpers ----------
static inline uint8_t to_bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }
static inline uint8_t from_bcd(uint8_t b) { return (uint8_t)(((b >> 4) * 10) + (b & 0x0F)); }

// Convert Y/M/D to days since 1970-01-01 (UTC). Valid 2000..2099.
static uint32_t ymd_to_days(uint16_t y, uint8_t m, uint8_t d) {
  // Shift March to be month 1
  if (m <= 2) { y -= 1; m += 12; }
  uint32_t era = y / 400;
  uint32_t yoe = (uint32_t)(y - era * 400);
  uint32_t doy = (153 * (m - 3) + 2) / 5 + d - 1;
  uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  // Days since 0000-03-01; convert to days since 1970-01-01
  const int32_t days_to_unix_epoch = 719468; // days from 0000-03-01 to 1970-01-01
  return era * 146097 + doe - days_to_unix_epoch;
}

// Compute weekday 1..7 (DS3231 expects 1=Sunday .. 7=Saturday)
static uint8_t weekday_1_7(uint16_t y, uint8_t m, uint8_t d) {
  // Zeller congruence (Sunday=1)
  if (m < 3) { m += 12; y -= 1; }
  uint16_t K = y % 100;
  uint16_t J = y / 100;
  int h = (d + (13 * (m + 1)) / 5 + K + (K / 4) + (J / 4) + (5 * J)) % 7; // 0=Saturday
  int dow = ((h + 6) % 7) + 1; // 1=Sunday..7=Saturday
  return (uint8_t)dow;
}

// ---------- DS3231 Low-level ----------
static void rtc_write_reg(uint8_t reg, uint8_t val) {
  bus_i2c_lock();
  Wire.beginTransmission(I2C_ADDR_DS3231);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
  bus_i2c_unlock();
}

static uint8_t rtc_read_reg(uint8_t reg) {
  bus_i2c_lock();
  Wire.beginTransmission(I2C_ADDR_DS3231);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((int)I2C_ADDR_DS3231, 1);
  if (Wire.available()) { uint8_t v = Wire.read(); bus_i2c_unlock(); return v; }
  bus_i2c_unlock();
  return 0xFF;
}

void rtc_begin() {
  // Control register 0x0E: INTCN=0 (SQW enabled), RS2=0, RS1=0 => 1 Hz, EOSC=0 (oscillator on)
  rtc_write_reg(0x0E, 0x00);
  // Clear OSF (Oscillator Stop Flag) in status 0x0F bit7
  uint8_t status = rtc_read_reg(0x0F);
  status &= ~(1u << 7);
  rtc_write_reg(0x0F, status);
}

void rtc_sync_from_gps(uint32_t unix_epoch) {
  // Convert UNIX epoch to date/time
  uint32_t days = unix_epoch / 86400UL;
  uint32_t rem = unix_epoch % 86400UL;
  uint8_t hh = rem / 3600UL; rem %= 3600UL;
  uint8_t mm = rem / 60UL;   uint8_t ss = rem % 60UL;

  // Convert days since 1970-01-01 to Y/M/D. Use algorithm inverse of ymd_to_days.
  int64_t z = (int64_t)days + 719468; // days since 0000-03-01
  int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  unsigned doe = (unsigned)(z - era * 146097);
  unsigned yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
  int y = (int)(yoe) + (int)(era) * 400;
  unsigned doy = doe - (365*yoe + yoe/4 - yoe/100);
  unsigned mp = (5*doy + 2)/153;
  unsigned d = doy - (153*mp+2)/5 + 1;
  unsigned m = mp + 3 - 12*(mp/10);
  y = y + (mp/10);

  uint16_t year = (uint16_t)y; // full year, e.g. 2025
  uint8_t month = (uint8_t)m;  // 1..12
  uint8_t day   = (uint8_t)d;  // 1..31
  uint8_t wday  = weekday_1_7(year, month, day);

  // DS3231 expects 00..99 (offset 2000). Clamp to 2000..2099.
  uint8_t yy = (uint8_t)((year >= 2000) ? (year - 2000) : (year % 100));

  // Write starting at 0x00
  bus_i2c_lock();
  Wire.beginTransmission(I2C_ADDR_DS3231);
  Wire.write(0x00);
  Wire.write(to_bcd(ss));                   // seconds
  Wire.write(to_bcd(mm));                   // minutes
  Wire.write(to_bcd(hh) & 0x3F);            // hours 24h
  Wire.write(to_bcd(wday));                 // day of week (1..7)
  Wire.write(to_bcd(day));                  // date
  Wire.write(to_bcd(month) & 0x1F);         // month (century=0)
  Wire.write(to_bcd(yy));                   // year (00..99)
  Wire.endTransmission();
  bus_i2c_unlock();
}

uint32_t rtc_get_unix() {
  // Read 0x00..0x06
  bus_i2c_lock();
  Wire.beginTransmission(I2C_ADDR_DS3231);
  Wire.write(0x00);
  Wire.endTransmission(false);
  Wire.requestFrom((int)I2C_ADDR_DS3231, 7);
  if (Wire.available() < 7) { bus_i2c_unlock(); return 0; }

  uint8_t bcd_ss   = Wire.read();
  uint8_t bcd_mm   = Wire.read();
  uint8_t bcd_hh   = Wire.read();
  uint8_t bcd_wday = Wire.read(); (void)bcd_wday;
  uint8_t bcd_day  = Wire.read();
  uint8_t bcd_mon  = Wire.read();
  uint8_t bcd_yy   = Wire.read();
  bus_i2c_unlock();

  uint8_t ss = from_bcd(bcd_ss & 0x7F);
  uint8_t mm = from_bcd(bcd_mm & 0x7F);
  uint8_t hh = from_bcd(bcd_hh & 0x3F);
  uint8_t day= from_bcd(bcd_day & 0x3F);
  uint8_t mon= from_bcd(bcd_mon & 0x1F);
  uint16_t year = 2000 + from_bcd(bcd_yy);

  uint32_t days = ymd_to_days(year, mon, day);
  uint32_t secs = days * 86400UL + (uint32_t)hh * 3600UL + (uint32_t)mm * 60UL + ss;
  return secs;
}
