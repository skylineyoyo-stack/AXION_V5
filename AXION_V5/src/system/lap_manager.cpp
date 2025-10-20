// AXION V4 — Lap manager (simple geofence + ghost)

#include <Arduino.h>
#include <math.h>
#include "sensor_structs.h"
#include "eeprom_at24c32.h"

struct TrackPoint { double lat, lon; };
static TrackPoint track[64];
static int track_n = 0;
static bool learning = true;
static uint32_t lap_start_ms = 0;
static float best_lap_s = 0.0f;

// Very simple geofence: circular radius around first point (30 m)
static bool in_start_zone(double lat, double lon) {
  if (track_n == 0) return false;
  double dlat = (lat - track[0].lat) * 111320.0; // rough meters/deg
  double dlon = (lon - track[0].lon) * 40075000.0 * cos(lat*3.14159/180.0) / 360.0;
  double d2 = dlat*dlat + dlon*dlon;
  return d2 < (30.0*30.0);
}

void lap_learn_tick() {
  if (!learning) return;
  if (g_gps_data.lock && g_gps_data.speed_mps > 5.0f) {
    static uint32_t t_last = 0;
    if (millis() - t_last > 1000 && track_n < 64) {
      track[track_n++] = { g_gps_data.lat, g_gps_data.lon };
      t_last = millis();
    }
  }
}

bool lap_detect_start_finish(bool& crossed) {
  static bool was_inside = false;
  bool inside = in_start_zone(g_gps_data.lat, g_gps_data.lon);
  crossed = (!was_inside && inside && g_gps_data.speed_mps > 5.5f);
  was_inside = inside;
  return crossed;
}

void lap_reset_best() { best_lap_s = 0.0f; }
float lap_get_best() { return best_lap_s; }
void lap_set_best(float s) { best_lap_s = s; }
bool lap_is_learning() { return learning; }
void lap_set_learning(bool b) { learning = b; }
void lap_start_timer() { lap_start_ms = millis(); }
float lap_current_time_s() { return (millis() - lap_start_ms) * 0.001f; }

// Persistence: pack lat/lon as int32 scaled 1e-7 deg
static constexpr uint16_t LAP_SLOT_SIZE = 0x400; // bytes per slot
static constexpr uint16_t LAP_EE_BASE = 0x200;
static int selected_slot = 1; // 1..3

bool lap_save_to_eeprom() {
  if (selected_slot < 1 || selected_slot > 3) selected_slot = 1;
  uint16_t base = LAP_EE_BASE + (selected_slot - 1) * LAP_SLOT_SIZE;
  struct Header { uint32_t magic; uint16_t count; uint16_t pad; } h{0x4C415031, (uint16_t)track_n, 0}; // 'LAP1'
  const size_t bytes = sizeof(Header) + track_n * 8;
  uint8_t* buf = (uint8_t*)malloc(bytes);
  if (!buf) return false;
  memcpy(buf, &h, sizeof h);
  int32_t* p = (int32_t*)(buf + sizeof h);
  for (int i=0;i<track_n;i++) {
    p[i*2+0] = (int32_t)(track[i].lat * 1e7);
    p[i*2+1] = (int32_t)(track[i].lon * 1e7);
  }
  bool ok = eeprom_at24c32_write(base, buf, bytes);
  free(buf);
  return ok;
}

bool lap_load_from_eeprom() {
  // Load the first valid slot 1..3
  for (int slot=1; slot<=3; ++slot) {
    uint16_t base = LAP_EE_BASE + (slot - 1) * LAP_SLOT_SIZE;
    struct Header { uint32_t magic; uint16_t count; uint16_t pad; } h{};
    if (!eeprom_at24c32_read(base, (uint8_t*)&h, sizeof h)) continue;
    if (h.magic != 0x4C415031) continue;
    if (h.count == 0 || h.count > 64) continue;
    size_t bytes = h.count * 8;
    int32_t* p = (int32_t*)malloc(bytes);
    if (!p) return false;
    bool ok = eeprom_at24c32_read(base + sizeof h, (uint8_t*)p, bytes);
    if (ok) {
      track_n = h.count;
      for (int i=0;i<track_n;i++) { track[i].lat = p[i*2+0] / 1e7; track[i].lon = p[i*2+1] / 1e7; }
      learning = false; selected_slot = slot; free(p); return true;
    }
    free(p);
  }
  return false;
}

// Slot selection API
void lap_set_slot(int slot) { if (slot<1) slot=1; if (slot>3) slot=3; selected_slot = slot; }
int  lap_get_slot() { return selected_slot; }
