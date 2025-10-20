// AXION V4 — UI: Diagnostics OBD2

#include <Arduino.h>
#include "axion.h"
#include "sensor_structs.h"

void ui_diagnostics() {
  oled_clear();
  oled_draw_text(2, 2, "OBD2 DIAGNOSTICS");
  char buf[64];
  snprintf(buf, sizeof(buf), "OBD: %s", g_vehicle_data.obd_ok?"OK":"--");
  oled_draw_text(2, 14, buf);
  snprintf(buf, sizeof(buf), "RPM: %u  THR:%d%%", g_vehicle_data.rpm, (int)g_vehicle_data.throttle);
  oled_draw_text(2, 24, buf);
  snprintf(buf, sizeof(buf), "vOBD: %.1f km/h", g_vehicle_data.speed_mps*3.6f);
  oled_draw_text(2, 34, buf);
  oled_blit();
}
