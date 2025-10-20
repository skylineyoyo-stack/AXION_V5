// AXION V4 — Menu manager (basic nav)

#include <Arduino.h>
#include "axion.h"
#include "ui_definitions.h"

static uint32_t t_btn_ms = 0;

void menu_init() {
  t_btn_ms = millis();
}

void menu_update() {
  uint32_t now = millis();
  if (now - t_btn_ms < 150) return; // debounce

  // Quick Return: SELECT long -> Dashboard
  static bool sel_hold = false; static uint32_t sel_t0 = 0;
  if (btn_select() && !sel_hold) { sel_hold = true; sel_t0 = now; }
  if (!btn_select() && sel_hold) { sel_hold = false; }
  if (sel_hold && (now - sel_t0 > 1500)) { ui_transition_to(UIMode::Dashboard, false); sel_hold = false; }

  if (btn_up()) {
    power_note_activity();
    uint8_t m = (uint8_t)g_mode;
    if (m == 0) m = (uint8_t)UIMode::Count - 1; else m--;
    ui_transition_to((UIMode)m, false);
    t_btn_ms = now;
  } else if (btn_down()) {
    power_note_activity();
    uint8_t m = ((uint8_t)g_mode + 1) % (uint8_t)UIMode::Count;
    ui_transition_to((UIMode)m, true);
    t_btn_ms = now;
  } else if (btn_select()) {
    power_note_activity();
    buzzer_beep(3000, 30, 64);
    t_btn_ms = now;
  }
}
