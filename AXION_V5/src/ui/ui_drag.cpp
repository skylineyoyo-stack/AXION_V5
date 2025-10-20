// AXION V4 — UI: Drag & Launch (1/8, 1/4, 1000ft)

#include <Arduino.h>
#include "axion.h"
#include "feedback.h"
#include "sensor_structs.h"

enum class DragMode { Eighth, Quarter, Thousand };

static DragMode dmode = DragMode::Quarter;
static drag_run_t run;

static float target_dist_m() {
  switch (dmode) {
    case DragMode::Eighth:   return 201.168f;   // 1/8 mile
    case DragMode::Thousand: return 304.800f;   // 1000 ft
    case DragMode::Quarter:  default: return 402.336f; // 1/4 mile
  }
}

static void countdown_sequence() {
  // Wait for next PPS then 3-2-1-GO with LED/buzzer at each PPS
  if (!run.countdown) { run.countdown = true; }
  static int step = 3;
  if (g_flag_pps) {
    g_flag_pps = false;
    if (step > 0) {
      // LED red for 3,2,1
      led_set(true, false);
      buzzer_beep(2000, 40, 64);
      step--;
    } else {
      // GO!
      led_set(false, true);
      buzzer_beep(2800, 60, 96);
      run.armed = true; run.countdown = false; step = 3;
    }
  }
}

void ui_drag() {
  float v_mps = g_gps_data.speed_mps;
  float v_kmh = v_mps * 3.6f;
  float gx = g_imu_data.ax;
  float dist_target = target_dist_m();

  // Inputs: UP/DOWN to switch distance when not running; SELECT to start countdown
  if (!run.running && !run.countdown && !run.finished) {
    if (btn_up()) dmode = (dmode == DragMode::Eighth ? DragMode::Quarter : (dmode == DragMode::Quarter ? DragMode::Thousand : DragMode::Eighth));
    if (btn_down()) dmode = (dmode == DragMode::Thousand ? DragMode::Quarter : (dmode == DragMode::Quarter ? DragMode::Eighth : DragMode::Thousand));
    if (btn_select()) { countdown_sequence(); }
  }
  if (run.countdown) countdown_sequence();

  // Start detection (READY->GO): G_long > 0.3 and v_gps > 1 km/h
  if (!run.running && run.armed) {
    if (gx > 0.30f && v_kmh > 1.0f) {
      run.running = true; run.finished = false;
      run.t_start = run.t_last = millis();
      run.dist_m = 0.0f; run.v_end_mps = 0.0f; run.gmax = 0.0f;
      run.t_60ft = run.t_330ft = run.t_660ft = run.t_final = 0;
      led_set(false, true);
#ifdef USE_SD
      session_start_drag();
#endif
      feedback_emit(UIMode::Drag, FbEvent::GO);
    }
  }

  // Integrate distance while running (10-30 Hz loop)
  if (run.running) {
    uint32_t now = millis();
    float dt = (now - run.t_last) * 1e-3f;
    run.t_last = now;
    run.dist_m += v_mps * dt;
    if (fabsf(gx) > run.gmax) run.gmax = fabsf(gx);
    run.v_end_mps = v_mps;

    if (run.t_60ft == 0 && run.dist_m >= 18.288f) run.t_60ft = now - run.t_start;
    if (run.t_330ft == 0 && run.dist_m >= 100.584f) run.t_330ft = now - run.t_start;
    if (run.t_660ft == 0 && run.dist_m >= 201.168f) run.t_660ft = now - run.t_start;
    if (run.dist_m >= dist_target) { run.t_final = now - run.t_start; run.running = false; run.finished = true; run.armed = false; led_set(false, false);
#ifdef USE_SD
      session_stop_drag();
#endif
      feedback_emit(UIMode::Drag, FbEvent::FINISH);
    }
  }

  // UI
  oled_clear();
  ui_draw_topbar("DRAG");
  oled_set_font_medium();
  char line[64];
  const char* mode = (dmode==DragMode::Eighth?"1/8":(dmode==DragMode::Quarter?"1/4":"1000ft"));
  snprintf(line, sizeof line, "Mode: %s  v=%.1f", mode, v_kmh);
  oled_draw_string(2, 14, line, 12);

  if (!run.running && !run.countdown && !run.finished) {
    oled_draw_string(2, 26, "UP/DOWN distance  SELECT=arm", 11);
  } else if (run.countdown) {
    oled_draw_string(2, 26, "Countdown... (PPS)", 14);
  } else if (run.running) {
    snprintf(line, sizeof line, "t=%.2fs d=%.1fm Gmax=%.2f", (millis()-run.t_start)/1000.0f, run.dist_m, run.gmax);
    oled_draw_string(2, 26, line, 13);
    snprintf(line, sizeof line, "60ft:%4ums 330:%4ums 660:%4ums", run.t_60ft, run.t_330ft, run.t_660ft);
    oled_draw_string(2, 36, line, 10);
  } else if (run.finished) {
    snprintf(line, sizeof line, "FINAL: %4ums  v_end=%.1f", run.t_final, run.v_end_mps*3.6f);
    oled_draw_string(2, 26, line, 15);
    snprintf(line, sizeof line, "60:%4u 330:%4u 660:%4u", run.t_60ft, run.t_330ft, run.t_660ft);
    oled_draw_string(2, 36, line, 12);
    // Prompt Save Run? YES/NO with intensity highlight
    static int sel = 0; // 0=YES 1=NO
    oled_draw_string(2, 46, "Save Run?", 11);
    oled_draw_string(90, 46, sel==0?"> YES":"  YES", sel==0?15:9);
    oled_draw_string(150,46, sel==1?"> NO":"  NO",  sel==1?15:9);
    if (btn_up()) { sel = 0; }
    if (btn_down()) { sel = 1; }
    if (btn_select()) {
#ifdef USE_SD
      if (sel==1) { session_discard_last(); }
#endif
      run = drag_run_t{};
    }
  }

  oled_blit();
}
