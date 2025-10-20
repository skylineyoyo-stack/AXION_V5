// AXION V4 — UI: System Health

#include <Arduino.h>
#include "axion.h"
#include "sensor_structs.h"
#include "system_state.h"
#include "error_logger.h"

void ui_systemhealth() {
  static bool view_log=false;
  static bool sel_hold=false; static uint32_t sel_t0=0;
  oled_clear();
  oled_draw_text(2, 2, view_log?"ERROR LOG (last 10)":"SYSTEM HEALTH");
  char buf[64];
  if (!view_log) {
    snprintf(buf, sizeof buf, "Uptime: %lus", (unsigned long)(millis()/1000));
    oled_draw_text(2, 14, buf);
    snprintf(buf, sizeof buf, "Free heap: %u", (unsigned)ESP.getFreeHeap());
    oled_draw_text(2, 24, buf);
#ifdef V_BATT_ADC_PIN
    {
      uint16_t raw = analogRead(V_BATT_ADC_PIN);
      float v = (float)raw * (3.3f / 4095.0f);
      #ifdef VBATT_DIV_RATIO
        v *= VBATT_DIV_RATIO;
      #endif
      snprintf(buf, sizeof buf, "Vbat: %.2f V", v);
      oled_draw_text(2, 34, buf);
    }
#else
    oled_draw_text(2, 34, "Vbat: N/A (define V_BATT_ADC_PIN)");
#endif
    SystemState s{}; system_state_load(s);
    snprintf(buf, sizeof buf, "Last run: %lus Boots:%lu", (unsigned long)(s.last_uptime_ms/1000), (unsigned long)s.boots);
    oled_draw_text(2, 44, buf);
    snprintf(buf, sizeof buf, "PPS:%d SQW:%d IMU:%d OBD:%d", (int)g_flag_pps, (int)g_flag_sqw, (millis()-g_imu_data.last_ms<500), (int)obd2_is_connected());
    oled_draw_text(2, 54, buf);
    // Enter Error Log with DOWN+SELECT long
    if (btn_select() && !sel_hold) { sel_hold=true; sel_t0=millis(); }
    if (!btn_select() && sel_hold) { sel_hold=false; }
    if (sel_hold && (millis()-sel_t0>800) && btn_down()) { view_log=true; sel_hold=false; }
  } else {
    ErrorEntry e[10]; uint16_t n = error_log_copy_last(e, 10);
    for (uint16_t i=0;i<n;i++) {
      snprintf(buf, sizeof buf, "%lu c%u s%u", (unsigned long)e[i].epoch, e[i].code, e[i].src);
      oled_draw_text(2, 14 + (int)i*9, buf);
    }
    oled_draw_text(2, 54, "SELECT exit");
    if (btn_select()) { view_log=false; }
  }
  oled_blit();
}
