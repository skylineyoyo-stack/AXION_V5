// AXION V4 — UI: Settings (basic)

#include <Arduino.h>
#include "axion.h"
#include "sensor_structs.h"
#include "eeprom_at24c32.h"
#include "feedback.h"
#include "feedback_settings.h"

void ui_settings() {
  static enum { VIEW_MAIN, VIEW_OBD_SCAN, VIEW_MAG_CAL, VIEW_FEEDBACK } view = VIEW_MAIN;
  static uint32_t debounce_ms = 0;
  static int sel = 0;

  auto debounced = [](){ uint32_t ms=millis(); if (ms - debounce_ms > 160) { debounce_ms = ms; return true; } return false; };

  oled_clear();
  char buf[64];

  static bool sel_down = false; static uint32_t sel_t0 = 0;
  if (view == VIEW_MAIN) {
    oled_draw_text(2, 2, "SETTINGS / STATS");
    snprintf(buf, sizeof(buf), "GPS sats:%u HDOP:%.1f lock:%s", g_gps_data.sats, g_gps_data.hdop, g_gps_data.lock?"Y":"N");
    oled_draw_text(2, 14, buf);
    uint32_t epoch = rtc_get_unix();
    snprintf(buf, sizeof(buf), "RTC: %lus", (unsigned long)epoch);
    oled_draw_text(2, 24, buf);
    oled_draw_text(2, 34, obd2_is_connected()?"OBD2: CONNECTED":"OBD2: DISCONNECTED");
    oled_draw_text(2, 44, "SELECT: OBD Pair | UP: Forget");
    oled_draw_text(2, 54, "DOWN: Calib Magneto | SELECT long: Feedback");
    if (btn_select() && debounced()) { view = VIEW_OBD_SCAN; sel = 0; obd2_scan_devices_start(); }
    else if (btn_up() && debounced()) { obd2_forget_saved(); }
    else if (btn_down() && debounced()) { view = VIEW_MAG_CAL; sel = 0; }
    // SELECT long opens Feedback page
    if (btn_select() && !sel_down) { sel_down = true; sel_t0 = millis(); }
    if (!btn_select() && sel_down) { sel_down = false; }
    if (sel_down && (millis() - sel_t0 > 800)) { view = VIEW_FEEDBACK; sel_down=false; }
    // SELECT long = deep sleep request
    if (btn_select() && !sel_down) { sel_down = true; sel_t0 = millis(); }
    if (!btn_select() && sel_down) { sel_down = false; }
    if (sel_down && (millis() - sel_t0 > 2000)) { power_request_deep_sleep(); sel_down = false; }
  } else if (view == VIEW_OBD_SCAN) {
    oled_draw_text(2, 2, "OBD2 SCAN");
    if (obd2_scan_in_progress()) {
      oled_draw_text(2, 14, "Scanning...");
    }
    size_t n = obd2_scan_count();
    for (size_t i=0; i<n && i<5; ++i) {
      char name[32]; uint8_t mac[6];
      obd2_scan_get(i, name, sizeof name, mac);
      snprintf(buf, sizeof buf, "%c %02X:%02X:%02X:%02X:%02X:%02X %s", (sel==(int)i?'>':' '),
               mac[0],mac[1],mac[2],mac[3],mac[4],mac[5], name);
      oled_draw_text(2, 14 + (int)i*10, buf);
    }
    oled_draw_text(2, 54, "UP/DOWN select  SELECT connect");
    if (btn_up() && debounced()) { if (sel>0) sel--; }
    if (btn_down() && debounced()) { if (sel<(int)n-1) sel++; }
    if (btn_select() && debounced()) {
      if (n>0 && sel < (int)n) { obd2_connect_index((size_t)sel); view = VIEW_MAIN; }
    }
  } else if (view == VIEW_MAG_CAL) {
    oled_draw_text(2, 2, "MAG CALIB (rotate 360)");
    static float minv[3] = { 1e9f, 1e9f, 1e9f};
    static float maxv[3] = {-1e9f,-1e9f,-1e9f};
    static int samples = 0;
    // Accumulate 200 samples
    if (samples < 200) {
      minv[0] = min(minv[0], g_imu_data.mx); maxv[0] = max(maxv[0], g_imu_data.mx);
      minv[1] = min(minv[1], g_imu_data.my); maxv[1] = max(maxv[1], g_imu_data.my);
      minv[2] = min(minv[2], g_imu_data.mz); maxv[2] = max(maxv[2], g_imu_data.mz);
      samples++;
    }
    snprintf(buf, sizeof buf, "N=%d  mx[%.1f %.1f]", samples, minv[0], maxv[0]); oled_draw_text(2, 14, buf);
    snprintf(buf, sizeof buf, "my[%.1f %.1f]", minv[1], maxv[1]); oled_draw_text(2, 24, buf);
    snprintf(buf, sizeof buf, "mz[%.1f %.1f]", minv[2], maxv[2]); oled_draw_text(2, 34, buf);
    if (samples >= 200) {
      float offs[3] = { (maxv[0]+minv[0])*0.5f, (maxv[1]+minv[1])*0.5f, (maxv[2]+minv[2])*0.5f };
      float scale[3]= { (maxv[0]-minv[0])*0.5f, (maxv[1]-minv[1])*0.5f, (maxv[2]-minv[2])*0.5f };
      // Save to EEPROM @0x100
      uint8_t bufw[24]; memcpy(bufw, offs, 12); memcpy(bufw+12, scale, 12);
      eeprom_at24c32_write(0x100, bufw, sizeof bufw);
      oled_draw_text(2, 46, "Saved. SELECT to exit");
      if (btn_select() && debounced()) { feedback_emit(UIMode::Compass, FbEvent::OK); view = VIEW_MAIN; samples = 0; for(int i=0;i<3;++i){minv[i]=1e9f;maxv[i]=-1e9f;} }
    } else {
      oled_draw_text(2, 46, "Rotate device slowly...");
    }
  }
  else if (view == VIEW_FEEDBACK) {
    oled_draw_text(2, 2, "FEEDBACK PREFS");
    static int row = 0; // 0=GLED 1=GBZ 2=LED 3=BZ
    static bool gled=true, gbz=true, led_on=true, bz_on=true; static bool init=true;
    static FeedbackPrefs fp{};
    if (init) { feedback_settings_load(fp); gled=fp.global_led; gbz=fp.global_buzzer; led_on=fp.enable_led[(uint8_t)g_mode]; bz_on=fp.enable_buzzer[(uint8_t)g_mode]; init=false; }
    char b2[64];
    snprintf(b2, sizeof b2, "Mode %u", (unsigned)g_mode+1); oled_draw_text(2, 14, b2);
    oled_draw_text(2, 24, row==0?"> GLOBAL LED":"  GLOBAL LED"); oled_draw_text(120,24, gled?"ON":"OFF");
    oled_draw_text(2, 33, row==1?"> GLOBAL BUZZER":"  GLOBAL BUZZER"); oled_draw_text(120,33, gbz?"ON":"OFF");
    oled_draw_text(2, 42, row==2?"> MODE LED":"  MODE LED"); oled_draw_text(120,42, led_on?"ON":"OFF");
    oled_draw_text(2, 51, row==3?"> MODE BUZZER":"  MODE BUZZER"); oled_draw_text(120,51, bz_on?"ON":"OFF");
    if (btn_up() && debounced()) { row = (row+3)%4; }
    if (btn_down() && debounced()) { row = (row+1)%4; }
    if (btn_select() && debounced()) {
      if (row==0) gled=!gled; else if (row==1) gbz=!gbz; else if (row==2) led_on=!led_on; else bz_on=!bz_on;
      feedback_set_global(gled, gbz);
      feedback_set_enable(g_mode, led_on, bz_on);
    }
    // SAVE with SELECT long
    if (btn_select() && !sel_down) { sel_down = true; sel_t0 = millis(); }
    if (!btn_select() && sel_down) { sel_down = false; }
    if (sel_down && (millis() - sel_t0 > 1000)) {
      fp.global_led = gled; fp.global_buzzer = gbz; fp.enable_led[(uint8_t)g_mode]=led_on; fp.enable_buzzer[(uint8_t)g_mode]=bz_on;
      feedback_settings_save(fp); feedback_settings_apply(fp);
      feedback_emit(UIMode::Settings, FbEvent::OK);
      view = VIEW_MAIN; init=true; // reload next time
    }
  }
  oled_blit();
}
