// AXION V4 — Test Mode overlay UI
#ifndef NO_TEST_MODE

#include <Arduino.h>
#include "axion.h"
#include "sensor_structs.h"

static bool g_testmode = false;
static uint32_t g_frame_ms = 0;
static uint16_t g_fps = 0;

void testmode_set(bool on) { g_testmode = on; }
bool testmode_get() { return g_testmode; }

void testmode_frame_tick() {
  static uint32_t last = 0; static uint16_t cnt = 0; uint32_t now = millis(); cnt++; if (now - last >= 1000) { g_fps = cnt; cnt = 0; last = now; }
}

void testmode_draw_overlay() {
  if (!g_testmode) return;
  char buf[48];
  oled_set_font_small();
  snprintf(buf, sizeof buf, "FPS:%u", (unsigned)g_fps); oled_draw_string(2, 54, buf, 12);
  snprintf(buf, sizeof buf, "Heap:%u", (unsigned)ESP.getFreeHeap()); oled_draw_string(60, 54, buf, 10);
  long dlat = (long)((long)g_ts_pps_us - (long)g_ts_sqw_us);
  snprintf(buf, sizeof buf, "PPS:%ld", dlat); oled_draw_string(130, 54, buf, 11);
  snprintf(buf, sizeof buf, "Err:%u", (unsigned)error_log_count()); oled_draw_string(200, 54, buf, 14);
}

#endif
