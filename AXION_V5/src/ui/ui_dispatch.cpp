// AXION V4 — UI dispatcher

#include <Arduino.h>
#include "axion.h"
#include "ui_definitions.h"

// Prototypes for each mode renderer
void ui_dashboard();
void ui_gforces();
void ui_accel();
void ui_drag();
void ui_lap();
void ui_crawling();
void ui_drift();
void ui_compass();
void ui_diagnostics();
void ui_settings();
void ui_bench();
void ui_systemhealth();

void ui_render_current() {
  // For now, render a distinct grayscale per mode (placeholder)
  switch (g_mode) {
    case UIMode::Dashboard:   ui_dashboard(); break;
    case UIMode::GForces:     ui_gforces(); break;
    case UIMode::Accel:       ui_accel(); break;
    case UIMode::Drag:        ui_drag(); break;
    case UIMode::Lap:         ui_lap(); break;
    case UIMode::Crawling:    ui_crawling(); break;
    case UIMode::Drift:       ui_drift(); break;
    case UIMode::Compass:     ui_compass(); break;
    case UIMode::Diagnostics: ui_diagnostics(); break;
    case UIMode::Settings:    ui_settings(); break;
    case UIMode::Bench:       ui_systemhealth(); break;
    default:                  oled_fill(0x0); break;
  }
  // Test mode overlay for all screens
  extern void toast_draw_if_any();
#ifndef NO_TEST_MODE
  extern void testmode_frame_tick();
  extern void testmode_draw_overlay();
  testmode_frame_tick();
  testmode_draw_overlay();
#endif
  toast_draw_if_any();
}
