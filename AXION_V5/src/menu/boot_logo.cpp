// AXION V4 — Boot logo (simple gradient flash)

#include <Arduino.h>
#include "axion.h"

static void gradient() {
  for (uint8_t g = 0; g <= 0x0F; ++g) {
    oled_fill(g);
    delay(10);
  }
  for (int g = 0x0F; g >= 0; --g) {
    oled_fill((uint8_t)g);
    delay(5);
  }
}

// Optionally call from axion_setup() if you want a boot splash
// Not auto-called to keep boot snappy.

