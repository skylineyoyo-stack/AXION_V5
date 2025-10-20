// AXION V4 — All pin and address definitions
#pragma once

#include <Arduino.h>

// I2C
static constexpr uint8_t PIN_I2C_SDA = 21;
static constexpr uint8_t PIN_I2C_SCL = 22;

// SPI (SSD1322)
static constexpr uint8_t PIN_SPI_MOSI = 23;
static constexpr uint8_t PIN_SPI_SCLK = 18;
static constexpr uint8_t PIN_OLED_DC   = 19; // DC pin (direct GPIO)

// UART2 (GPS)
static constexpr uint8_t PIN_GPS_RX2 = 16; // ESP32 RX2
static constexpr uint8_t PIN_GPS_TX2 = 17; // ESP32 TX2
static constexpr uint8_t PIN_GPS_PPS = 26; // PPS interrupt

// Interrupts
static constexpr uint8_t PIN_INT_MPU = 33;
static constexpr uint8_t PIN_SQW_RTC = 32;

// Buzzer (PWM)
static constexpr uint8_t PIN_BUZZER = 25;
static constexpr uint8_t LEDC_CH_BUZZ = 0;

// I2C addresses
static constexpr uint8_t I2C_ADDR_PCF8574 = 0x20;
static constexpr uint8_t I2C_ADDR_EEPROM  = 0x50; // AT24C32
static constexpr uint8_t I2C_ADDR_DS3231  = 0x68;
static constexpr uint8_t I2C_ADDR_MPU9250 = 0x69;
static constexpr uint8_t I2C_ADDR_AK8963  = 0x0C; // magnetometer inside MPU9250

// PCF8574 bit mapping
// P0..P2: Buttons (active LOW)
// P3: LED red (LOW=ON)
// P4: LED green (LOW=ON)
// P5: OLED RESET (active LOW)
// P6: OLED CS (active LOW)
// P7: Reserved
static constexpr uint8_t PCF_BIT_BTN_UP     = 0;
static constexpr uint8_t PCF_BIT_BTN_DOWN   = 1;
static constexpr uint8_t PCF_BIT_BTN_SELECT = 2;
static constexpr uint8_t PCF_BIT_LED_RED    = 3;
static constexpr uint8_t PCF_BIT_LED_GREEN  = 4;
static constexpr uint8_t PCF_BIT_OLED_RST   = 5;
static constexpr uint8_t PCF_BIT_OLED_CS    = 6;

// SD card (SPI CS)
static constexpr uint8_t PIN_SD_CS = 5;
// SD card HSPI pins (recommended)
static constexpr uint8_t PIN_SD_SCK  = 14;
static constexpr uint8_t PIN_SD_MISO = 12;
static constexpr uint8_t PIN_SD_MOSI = 13;
