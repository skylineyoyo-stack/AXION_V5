// AXION V4 — IMU MPU9250 (basic bring-up + Madgwick IMU-only)

#include <Arduino.h>
#include <Wire.h>
#include "hardware/pins.h"
#include "axion.h"
#include "sensor_structs.h"
#include "system/filters/madgwick_ahrs.h"
#include "eeprom_at24c32.h"

static Madgwick ahrs;
static uint32_t last_us = 0;
static float mag_asa[3] = {1,1,1};
static float mag_offs[3] = {0,0,0};
static float mag_scale[3] = {1,1,1};
static bool s_mag_cal_ok = false;

static void load_mag_cal() {
  uint8_t buf[24];
  if (eeprom_at24c32_read(0x100, buf, sizeof buf)) {
    memcpy(mag_offs, buf, 12);
    memcpy(mag_scale, buf+12, 12);
    // Basic sanity
    for (int i=0;i<3;++i) {
      if (!isfinite(mag_scale[i]) || fabsf(mag_scale[i]) < 1e-6f) mag_scale[i] = 1.0f;
      if (!isfinite(mag_offs[i])) mag_offs[i] = 0.0f;
    }
    s_mag_cal_ok = true;
  }
}

static int i2c_wb(uint8_t addr, uint8_t reg, uint8_t val) {
  bus_i2c_lock();
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  int r = Wire.endTransmission();
  bus_i2c_unlock();
  return r;
}

static int i2c_rb(uint8_t addr, uint8_t reg, uint8_t* buf, size_t len) {
  bus_i2c_lock();
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) { bus_i2c_unlock(); return -1; }
  size_t n = Wire.requestFrom((int)addr, (int)len);
  if (n != len) { bus_i2c_unlock(); return -2; }
  for (size_t i = 0; i < len; ++i) buf[i] = Wire.read();
  bus_i2c_unlock();
  return 0;
}

void imu_begin() {
  // Reset and wake
  i2c_wb(I2C_ADDR_MPU9250, 0x6B, 0x80); delay(100);
  i2c_wb(I2C_ADDR_MPU9250, 0x6B, 0x01); // clock PLL
  delay(10);
  // DLPF ~184 Hz
  i2c_wb(I2C_ADDR_MPU9250, 0x1A, 0x01);
  // Sample rate: 1kHz / (1+4) = 200 Hz
  i2c_wb(I2C_ADDR_MPU9250, 0x19, 0x04);
  // Gyro ±500 dps
  i2c_wb(I2C_ADDR_MPU9250, 0x1B, 0x08);
  // Accel ±2g
  i2c_wb(I2C_ADDR_MPU9250, 0x1C, 0x00);
  // Accel DLPF ~184 Hz
  i2c_wb(I2C_ADDR_MPU9250, 0x1D, 0x01);

  // Enable I2C bypass to access AK8963 directly
  i2c_wb(I2C_ADDR_MPU9250, 0x37, 0x02);

  // AK8963 init: power-down
  i2c_wb(I2C_ADDR_AK8963, 0x0A, 0x00); delay(10);
  // Enter fuse ROM to read ASA
  i2c_wb(I2C_ADDR_AK8963, 0x0A, 0x0F); delay(10);
  uint8_t asa_raw[3] = {0};
  i2c_rb(I2C_ADDR_AK8963, 0x10, asa_raw, 3);
  for (int i=0;i<3;++i) mag_asa[i] = ((asa_raw[i]-128)/256.0f) + 1.0f;
  // Power-down then set to continuous measurement 2 (100 Hz), 16-bit
  i2c_wb(I2C_ADDR_AK8963, 0x0A, 0x00); delay(10);
  i2c_wb(I2C_ADDR_AK8963, 0x0A, 0x16); delay(10);

  ahrs.beta = 0.1f;
  last_us = micros();
  load_mag_cal();
}

void imu_tick() {
  uint8_t raw[14];
  if (i2c_rb(I2C_ADDR_MPU9250, 0x3B, raw, sizeof raw) != 0) return;
  int16_t ax = (int16_t)((raw[0] << 8) | raw[1]);
  int16_t ay = (int16_t)((raw[2] << 8) | raw[3]);
  int16_t az = (int16_t)((raw[4] << 8) | raw[5]);
  int16_t gx = (int16_t)((raw[8] << 8) | raw[9]);
  int16_t gy = (int16_t)((raw[10] << 8) | raw[11]);
  int16_t gz = (int16_t)((raw[12] << 8) | raw[13]);

  // Scale
  const float as = 1.0f / 16384.0f; // ±2g
  const float gs = 1.0f / 65.5f;    // ±500 dps
  float fax = ax * as;
  float fay = ay * as;
  float faz = az * as;
  float fgx = gx * gs * 0.0174532925f; // rad/s
  float fgy = gy * gs * 0.0174532925f;
  float fgz = gz * gs * 0.0174532925f;

  uint32_t now = micros();
  float dt = (now - last_us) * 1e-6f;
  if (dt <= 0 || dt > 0.1f) dt = 0.005f; // guard
  last_us = now;

  // Read AK8963 magnetometer (if data ready)
  // ST1 register (0x02) bit0 DRDY
  uint8_t st1 = 0;
  if (i2c_rb(I2C_ADDR_AK8963, 0x02, &st1, 1) == 0 && (st1 & 0x01)) {
    uint8_t mbuf[7];
    if (i2c_rb(I2C_ADDR_AK8963, 0x03, mbuf, 7) == 0) {
      int16_t mx = (int16_t)(mbuf[1] << 8 | mbuf[0]);
      int16_t my = (int16_t)(mbuf[3] << 8 | mbuf[2]);
      int16_t mz = (int16_t)(mbuf[5] << 8 | mbuf[4]);
      // ST2 in mbuf[6]; read to clear
      (void)mbuf[6];
      // Scale to uT approx and apply ASA
      const float mres = 0.15f; // uT/LSB for 16-bit
      float fmx = mx * mres * mag_asa[0];
      float fmy = my * mres * mag_asa[1];
      float fmz = mz * mres * mag_asa[2];
      // Apply hard/soft-iron calibration
      fmx = (fmx - mag_offs[0]) / mag_scale[0];
      fmy = (fmy - mag_offs[1]) / mag_scale[1];
      fmz = (fmz - mag_offs[2]) / mag_scale[2];
      // Update AHRS with mag
      ahrs.update(fgx, fgy, fgz, fax, fay, faz, fmx, fmy, fmz, dt);
      g_imu_data.mx = fmx; g_imu_data.my = fmy; g_imu_data.mz = fmz;
    } else {
      ahrs.updateIMU(fgx, fgy, fgz, fax, fay, faz, dt);
    }
  } else {
    ahrs.updateIMU(fgx, fgy, fgz, fax, fay, faz, dt);
  }
  float yaw, pitch, roll;
  ahrs.getEuler(yaw, pitch, roll);

  g_imu_data.ax = fax; g_imu_data.ay = fay; g_imu_data.az = faz;
  g_imu_data.gx = gx * gs; g_imu_data.gy = gy * gs; g_imu_data.gz = gz * gs;
  g_imu_data.yaw = yaw; g_imu_data.pitch = pitch; g_imu_data.roll = roll;
  g_imu_data.last_ms = millis();
}

bool imu_mag_calibrated() { return s_mag_cal_ok; }
