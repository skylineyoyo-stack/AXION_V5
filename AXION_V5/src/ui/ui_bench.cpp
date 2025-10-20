// AXION V4 — UI: Bench / Tests

#include <Arduino.h>
#include "axion.h"

void ui_bench() {
  oled_clear();
  oled_draw_text(2, 2, "BENCH");
  oled_draw_text(2, 14, "Perf/UI tests TBD");
  oled_blit();
}
