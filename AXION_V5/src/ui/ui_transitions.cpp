// AXION V4 — UI slide transitions using sprites (disabled in NO_TRANSITIONS)

#include "axion.h"
#include "ui_definitions.h"

// Mode renderers
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
void ui_systemhealth();

static void render_mode(UIMode m) {
  switch (m) {
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
    default: break;
  }
}

void ui_transition_to(UIMode newMode, bool forward) {
#ifdef NO_TRANSITIONS
  (void)forward; g_mode = newMode; return;
#else
  // Render current into sprite A
  oled_sprite_begin_A();
  render_mode(g_mode);
  oled_sprite_end();
  // Render next into sprite B
  oled_sprite_begin_B();
  UIMode old = g_mode; g_mode = newMode; render_mode(newMode); g_mode = old;
  oled_sprite_end();

  // Slide animation
  const int W = 256; const int H = 64; const int step = 16; // ~16 frames
  for (int off=0; off<=W; off+=step) {
    oled_clear();
    if (forward) {
      // old slides left, new enters from right
      oled_sprite_push_A(-off, 0);
      oled_sprite_push_B(W - off, 0);
    } else {
      // old slides right, new enters from left
      oled_sprite_push_A(off, 0);
      oled_sprite_push_B(-W + off, 0);
    }
    oled_blit();
    delay(10);
  }
  g_mode = newMode;
#endif
}
