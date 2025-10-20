// AXION V4 — UI: Lap Timer (learn + ghost)

#include <Arduino.h>
#include "axion.h"
#include "sensor_structs.h"
#include "feedback.h"

// lap_manager.cpp provides these helpers
void lap_learn_tick();
bool lap_detect_start_finish(bool& crossed);
void lap_reset_best();
float lap_get_best();
void lap_set_best(float s);
bool lap_is_learning();
void lap_set_learning(bool b);
void lap_start_timer();
float lap_current_time_s();

void ui_lap() {
  static bool timing = false;
  static uint32_t t_ui = 0;
  if (millis() - t_ui > 100) t_ui = millis();

  if (lap_is_learning()) lap_learn_tick();

  bool crossed = false;
  if (lap_detect_start_finish(crossed) && !lap_is_learning()) {
    if (!timing) { lap_start_timer(); timing = true; }
    else {
      float t = lap_current_time_s();
      // Lap complete
      timing = false;
      feedback_emit(UIMode::Lap, FbEvent::CROSSING);
      if (lap_get_best() == 0.0f || t < lap_get_best()) { lap_set_best(t); }
    }
  }

  // Controls:
  // SELECT toggles learn/race
  // In Learn: UP/DOWN cycle track slot 1..3; in Race: UP resets best
  static int slot = 1;
  if (lap_is_learning()) {
    if (btn_up())   { slot = (slot % 3) + 1; delay(200); }
    if (btn_down()) { slot = ((slot+1) % 3) + 1; delay(200); }
    lap_set_slot(slot);
  } else {
    if (btn_up()) { lap_reset_best(); delay(200); }
  }
  if (btn_select()) { lap_set_learning(!lap_is_learning()); if (!lap_is_learning()) lap_save_to_eeprom(); delay(200); }

  oled_clear();
  ui_draw_topbar("LAP");
  oled_set_font_medium();
  oled_draw_string(2, 12, lap_is_learning()?"LEARN":"RACE", 12);
  char buf[64];
  snprintf(buf, sizeof buf, "v=%.1f km/h", g_gps_data.speed_mps*3.6f);
  oled_draw_string(120, 12, buf, 10);

  if (!lap_is_learning()) {
    float cur = timing ? lap_current_time_s() : 0.0f;
    float best = lap_get_best();
    snprintf(buf, sizeof buf, "t=%.2fs best=%.2fs", cur, best);
    oled_draw_string(2, 26, buf, 11);
    if (best > 0.0f && timing) {
      float delta = cur - best;
      uint8_t g = (delta < -0.2f) ? 13 : (delta > 0.2f ? 5 : 9);
      snprintf(buf, sizeof buf, "dT: %+0.2fs", delta);
      oled_draw_string(2, 36, buf, g);
      // Simple progress bar by speed fraction toward typical track segment
      int w = (int)min(200.0f, (g_gps_data.speed_mps*3.6f));
      oled_fill_rect(2, 48, w, 4, g);
    }
  } else {
    snprintf(buf, sizeof buf, "Track slot: %d/3", slot);
    oled_draw_string(2, 26, buf, 9);
    oled_draw_string(2, 36, "Drive 1-2 laps to learn", 9);
  }

  oled_blit();
}
