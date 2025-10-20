// AXION V4 — LED/Buzzer feedback router (non-blocking)

#include <Arduino.h>
#include "axion.h"
#include "feedback.h"
#include "hardware/pins.h"
#include "sensor_structs.h"

struct LedPattern { uint16_t on_ms; uint16_t off_ms; uint8_t repeats; uint8_t color; bool active; uint32_t t_next; uint8_t count; bool state; };
struct BuzzPattern { uint16_t freq; uint16_t dur_ms; uint8_t notes[4]; uint8_t ncount; uint8_t idx; uint32_t t_next; bool active; };

static UIMode g_cur = UIMode::Dashboard;
static LedPattern g_led{}; // color: 1=green, 2=red, 3=both(yellow)
static BuzzPattern g_bz{};
static bool g_enable_led[ (uint8_t)UIMode::Count ];
static bool g_enable_bz[ (uint8_t)UIMode::Count ];
static bool g_dash_all_ok=false, g_dash_lost=false;
static bool g_global_led=true, g_global_bz=true;

static void led_apply(bool on, uint8_t color) {
  bool red = false, green = false;
  if (on) {
    if (color==1) green=true; else if (color==2) red=true; else { red=true; green=true; }
  }
  led_set(red, green);
}

void feedback_init() {
  memset(g_enable_led, 1, sizeof g_enable_led);
  memset(g_enable_bz, 1, sizeof g_enable_bz);
  g_led = {}; g_bz = {};
}

void feedback_set_mode(UIMode m) { g_cur = m; }

void feedback_set_enable(UIMode m, bool led, bool buzzer) {
  g_enable_led[(uint8_t)m] = led; g_enable_bz[(uint8_t)m] = buzzer;
}

void feedback_set_dashboard_status(bool all_ok, bool sensor_lost_long) {
  g_dash_all_ok = all_ok; g_dash_lost = sensor_lost_long;
}

void feedback_set_global(bool led, bool buzzer) {
  g_global_led = led; g_global_bz = buzzer;
}

static void schedule_led(uint16_t on_ms, uint16_t off_ms, uint8_t reps, uint8_t color) {
  g_led.on_ms=on_ms; g_led.off_ms=off_ms; g_led.repeats=reps; g_led.color=color; g_led.active=true; g_led.t_next=millis(); g_led.count=0; g_led.state=false;
}

static void schedule_buzz_notes(const uint8_t* notes, uint8_t n) {
  memset(&g_bz,0,sizeof g_bz); g_bz.active=true; g_bz.ncount=(n>4?4:n); memcpy(g_bz.notes, notes, g_bz.ncount); g_bz.idx=0; g_bz.t_next=millis();
}

void feedback_emit(UIMode m, FbEvent e) {
  if ((!g_global_led && !g_global_bz) || (!g_enable_led[(uint8_t)m] && !g_enable_bz[(uint8_t)m])) return;
  switch (m) {
    case UIMode::Dashboard:
      if (e==FbEvent::ERROR && g_enable_led[(uint8_t)m]) schedule_led(400,400,2,2);
      break;
    case UIMode::GForces:
      if (e==FbEvent::RECORD) {
        if (g_enable_led[(uint8_t)m]) schedule_led(150,150,3,1);
        if (g_enable_bz[(uint8_t)m]) { uint8_t n[3]={1,2,0}; schedule_buzz_notes(n,2); }
      }
      break;
    case UIMode::Accel:
      if (e==FbEvent::GO) { if (g_enable_led[(uint8_t)m]) schedule_led(250,250,20,3); } // blinking during accel
      if (e==FbEvent::FINISH) {
        if (g_enable_led[(uint8_t)m]) schedule_led(300,200,1,1);
        if (g_enable_bz[(uint8_t)m]) { uint8_t n[1]={1}; schedule_buzz_notes(n,1); }
      }
      break;
    case UIMode::Drag:
      if (e==FbEvent::GO) {
        if (g_enable_led[(uint8_t)m]) { g_led.active=false; led_apply(true,1); }
        if (g_enable_bz[(uint8_t)m]) { uint8_t n[1]={1}; schedule_buzz_notes(n,1); }
      } else if (e==FbEvent::FINISH) {
        if (g_enable_led[(uint8_t)m]) schedule_led(200,200,2,1);
        if (g_enable_bz[(uint8_t)m]) { uint8_t n[1]={1}; schedule_buzz_notes(n,1); }
      }
      break;
    case UIMode::Lap:
      if (e==FbEvent::CROSSING) {
        if (g_enable_led[(uint8_t)m]) schedule_led(200,0,1,3);
        if (g_enable_bz[(uint8_t)m]) { uint8_t n[1]={1}; schedule_buzz_notes(n,1); }
      } else if (e==FbEvent::BEST) {
        if (g_enable_led[(uint8_t)m]) schedule_led(150,150,3,1);
        if (g_enable_bz[(uint8_t)m]) { uint8_t n[3]={1,2,3}; schedule_buzz_notes(n,3); }
      }
      break;
    case UIMode::Drift:
      if (e==FbEvent::DRIFT_SCORE_HIGH) {
        if (g_enable_led[(uint8_t)m]) schedule_led(100,100,2,1);
        if (g_enable_bz[(uint8_t)m]) { uint8_t n[1]={2}; schedule_buzz_notes(n,1); }
      } else if (e==FbEvent::RECORD) {
        if (g_enable_led[(uint8_t)m]) schedule_led(100,80,3,1);
        if (g_enable_bz[(uint8_t)m]) { uint8_t n[3]={1,2,3}; schedule_buzz_notes(n,3); }
      }
      break;
    case UIMode::Diagnostics:
      if (e==FbEvent::CONNECT) {
        if (g_enable_led[(uint8_t)m] && g_global_led) { g_led.active=false; led_apply(true,1); }
        if (g_enable_bz[(uint8_t)m] && g_global_bz) { uint8_t n[1]={1}; schedule_buzz_notes(n,1); }
      } else if (e==FbEvent::DISCONNECT) {
        if (g_enable_led[(uint8_t)m] && g_global_led) { g_led.active=false; led_apply(true,2); }
        if (g_enable_bz[(uint8_t)m] && g_global_bz) { uint8_t n[1]={3}; schedule_buzz_notes(n,1); }
      }
      break;
    case UIMode::Settings:
      if (e==FbEvent::DEEP_SLEEP) { if (g_enable_bz[(uint8_t)m]) { uint8_t n[1]={2}; schedule_buzz_notes(n,1); } if (g_enable_led[(uint8_t)m]) schedule_led(500,0,1,2); }
      break;
    case UIMode::Compass:
      if (e==FbEvent::OK) { if (g_enable_led[(uint8_t)m] && g_global_led) { g_led.active=false; led_apply(true,1); } }
      if (e==FbEvent::ERROR) { if (g_enable_led[(uint8_t)m] && g_global_led) { g_led.active=false; led_apply(true,2); } }
      break;
    case UIMode::Crawling:
      // handled as baseline in tick
      break;
    case UIMode::Bench:
      // no direct events yet; baseline in tick
      break;
    default: break;
  }
}

void feedback_tick() {
  uint32_t now = millis();
  // Dashboard baseline
  if (g_cur == UIMode::Dashboard && g_enable_led[(uint8_t)UIMode::Dashboard]) {
    if (!g_led.active) {
      if (g_dash_all_ok) led_apply(true,1); else led_apply(false,1);
      if (g_dash_lost) { schedule_led(400,400,2,2); }
    }
  }
  // LED pattern state machine
  if (g_led.active && now >= g_led.t_next) {
    if (!g_led.state) {
      led_apply(true, g_led.color);
      g_led.state = true;
      g_led.t_next = now + g_led.on_ms;
    } else {
      led_apply(false, g_led.color);
      g_led.state = false;
      g_led.count++;
      if (g_led.count >= g_led.repeats) { g_led.active = false; }
      else g_led.t_next = now + g_led.off_ms;
    }
  }
  // Buzzer pattern (notes: 1,2,3 represent ascending tones)
  if (g_bz.active && now >= g_bz.t_next) {
    if (g_bz.idx < g_bz.ncount) {
      uint8_t n = g_bz.notes[g_bz.idx++];
      uint16_t freq = (n==1?2000:(n==2?2400:2800));
      ledcWriteTone(0, freq); ledcWrite(0, 128);
      g_bz.t_next = now + (n==3?150:100);
    } else {
      ledcWrite(0, 0); g_bz.active = false;
    }
  }

  // Baseline for Compass / Crawling / Health when no active pattern
  if (!g_led.active && g_global_led) {
    if (g_cur == UIMode::Compass) {
      if (imu_mag_calibrated()) led_apply(true,1); else led_apply(true,2);
    } else if (g_cur == UIMode::Crawling) {
      extern IMUData g_imu_data; float a = max(fabsf(g_imu_data.pitch), fabsf(g_imu_data.roll));
      const float ANGLE_WARN=20.0f, ANGLE_CRIT=30.0f;
      if (a < ANGLE_WARN) led_apply(true,1); else if (a < ANGLE_CRIT) led_apply(true,3); else led_apply(true,2);
      static uint32_t t_bip=0; if (a >= ANGLE_CRIT && g_global_bz && (now - t_bip > 3000)) { uint8_t n[2]={2,1}; schedule_buzz_notes(n,2); t_bip = now; }
    } else if (g_cur == UIMode::Bench) {
      bool all_ok = g_gps_data.hdop>0 && g_gps_data.hdop<2.0f && (millis()-g_imu_data.last_ms<500);
      led_apply(all_ok, all_ok?1:2);
    }
  }
}
