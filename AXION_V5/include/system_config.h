// AXION V4 — System configuration
#pragma once
#include <Arduino.h>


// Versioning & UI
static constexpr uint8_t AXION_MAJOR = 4;
static constexpr uint8_t AXION_MINOR = 0;

static constexpr uint16_t UI_FPS = 30;        // target FPS
static constexpr uint16_t LOGGER_HZ = 10;     // 10 Hz

// Themes / display
static constexpr uint8_t THEME_BG = 0x0;      // black
static constexpr uint8_t THEME_FG = 0xF;      // white (4-bit)
