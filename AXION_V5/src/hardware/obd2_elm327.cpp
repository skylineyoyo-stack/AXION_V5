// AXION V4 — OBD2 ELM327 via Bluetooth (stub)

#include <Arduino.h>
#include <BluetoothSerial.h>
#include "sensor_structs.h"
#include "eeprom_at24c32.h"
#include "hardware/pins.h"
#include <esp_gap_bt_api.h>

static BluetoothSerial SerialBT;
static bool bt_ready = false;
static uint32_t last_query_ms = 0;
static bool pairing_active = false;
static bool scanning_active = false;
struct ObdDev { char name[32]; uint8_t mac[6]; };
static ObdDev found[12];
static size_t found_n = 0;
static uint8_t saved_mac[6] = {0};
static bool mac_valid = false;
static uint8_t last_target_mac[6] = {0};

static constexpr uint16_t EEPROM_MAC_ADDR = 0x000; // 6 bytes + checksum

static void save_mac_to_eeprom(const uint8_t mac[6]) {
  uint8_t buf[7]; memcpy(buf, mac, 6); buf[6] = 0; for (int i=0;i<6;++i) buf[6] ^= mac[i];
  eeprom_at24c32_write(EEPROM_MAC_ADDR, buf, sizeof buf);
}

static bool load_mac_from_eeprom(uint8_t mac[6]) {
  uint8_t buf[7]; if (!eeprom_at24c32_read(EEPROM_MAC_ADDR, buf, sizeof buf)) return false;
  uint8_t cs = 0; for (int i=0;i<6;++i) cs ^= buf[i]; if (cs != buf[6]) return false;
  memcpy(mac, buf, 6); return true;
}

// Very basic SPP server start (for now). To connect to a specific ELM327, a client role with MAC is required.
static bool connect_mac(const uint8_t mac[6]) {
  memcpy(last_target_mac, mac, 6);
  uint8_t tmp[6]; memcpy(tmp, mac, 6);
  return SerialBT.connect(tmp);
}

bool obd2_is_connected() { return bt_ready && SerialBT.connected(); }

bool obd2_connect_saved() {
  if (!bt_ready) return false;
  if (!mac_valid) return false;
  bool ok = connect_mac(saved_mac);
  Serial.printf("OBD2 connect_saved: %s\n", ok?"OK":"fail");
  if (ok) save_mac_to_eeprom(saved_mac);
  return ok;
}

bool obd2_connect_by_name(const char* name) {
  if (!bt_ready) return false;
  bool ok = SerialBT.connect(String(name));
  Serial.printf("OBD2 connect name '%s': %s\n", name, ok?"OK":"fail");
  return ok;
}

// GAP callback for device discovery
static void gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  if (!pairing_active) return;
  if (event == ESP_BT_GAP_DISC_RES_EVT) {
    // Try to read BDNAME/EIR
    char name[64] = {0};
    for (int i = 0; i < param->disc_res.num_prop; ++i) {
      const esp_bt_gap_dev_prop_t& p = param->disc_res.prop[i];
      if (p.type == ESP_BT_GAP_DEV_PROP_BDNAME && p.val && p.len > 0) {
        size_t l = (p.len < 63) ? p.len : 63; memcpy(name, p.val, l); name[l] = 0;
      } else if (p.type == ESP_BT_GAP_DEV_PROP_EIR && p.val && p.len > 0 && name[0] == 0) {
        uint8_t *eir = (uint8_t*)p.val; uint8_t len = 0;
        uint8_t* d = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &len);
        if (!d) d = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &len);
        if (d && len > 0) { size_t l = (len < 63) ? len : 63; memcpy(name, d, l); name[l] = 0; }
      }
    }
    // Filter names
    if (name[0]) {
      String s(name);
      s.toUpperCase();
      if (s.indexOf("OBD") >= 0 || s.indexOf("ELM") >= 0 || s.indexOf("VLINK")>=0 || s.indexOf("CAN")>=0) {
        if (found_n < (sizeof(found)/sizeof(found[0]))) {
          strncpy(found[found_n].name, name, sizeof(found[found_n].name)-1);
          found[found_n].name[sizeof(found[0].name)-1] = 0;
          memcpy(found[found_n].mac, param->disc_res.bda, 6);
          ++found_n;
        }
        // Attempt connect by MAC
        esp_bd_addr_t addr; memcpy(addr, param->disc_res.bda, 6);
        Serial.printf("Pairing: found %s, trying connect...\n", name);
        if (connect_mac(addr)) {
          save_mac_to_eeprom(addr); memcpy(saved_mac, addr, 6); mac_valid = true; pairing_active = false;
          esp_bt_gap_cancel_discovery();
        }
      }
    }
  } else if (event == ESP_BT_GAP_DISC_STATE_CHANGED_EVT) {
    if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
      pairing_active = false; scanning_active = false;
    }
  }
}

bool obd2_start_pairing() {
  if (!bt_ready) return false;
  pairing_active = true; scanning_active = true; found_n = 0;
  esp_bt_gap_register_callback(gap_cb);
  esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
  Serial.println("OBD2 pairing started: scanning...");
  return true;
}

bool obd2_scan_devices_start() { return obd2_start_pairing(); }
bool obd2_scan_in_progress() { return scanning_active; }
size_t obd2_scan_count() { return found_n; }
bool obd2_scan_get(size_t idx, char* name_out, size_t name_max, uint8_t mac_out[6]) {
  if (idx >= found_n) return false;
  if (name_out && name_max) { strncpy(name_out, found[idx].name, name_max-1); name_out[name_max-1]=0; }
  if (mac_out) memcpy(mac_out, found[idx].mac, 6);
  return true;
}
bool obd2_connect_index(size_t idx) {
  if (idx >= found_n) return false;
  bool ok = connect_mac(found[idx].mac);
  if (ok) { save_mac_to_eeprom(found[idx].mac); memcpy(saved_mac, found[idx].mac, 6); mac_valid = true; }
  return ok;
}
bool obd2_forget_saved() {
  uint8_t zero[7]={0};
  bool ok = eeprom_at24c32_write(EEPROM_MAC_ADDR, zero, sizeof zero);
  mac_valid = false; memset(saved_mac,0,6);
  return ok;
}

void obd2_begin() {
  // Master mode to allow client connect to ELM327 SPP server
  bt_ready = SerialBT.begin("AXION", true);
  Serial.println(bt_ready ? "BT SPP master started" : "BT start failed");
  if (bt_ready) {
    if (load_mac_from_eeprom(saved_mac)) { mac_valid = true; Serial.println("OBD2 MAC loaded from EEPROM"); }
    if (mac_valid) { obd2_connect_saved(); }
  }
}

static void send_cmd(const char* s) { SerialBT.print(s); SerialBT.print("\r"); }

static bool readline(char* buf, size_t maxlen, uint32_t timeout_ms=50) {
  uint32_t t0 = millis(); size_t idx = 0;
  while (millis() - t0 < timeout_ms && idx < maxlen - 1) {
    while (SerialBT.available() && idx < maxlen - 1) {
      char c = (char)SerialBT.read();
      if (c == '\r') continue;
      if (c == '\n') { buf[idx] = 0; return idx > 0; }
      buf[idx++] = c;
    }
  }
  buf[idx] = 0; return idx > 0;
}

static bool parse_pid_010C(const char* line, uint16_t& rpm) {
  // Expecting like: 41 0C AA BB
  const char* p = strstr(line, "41 0C");
  if (!p) return false;
  unsigned a, b; if (sscanf(p, "41 0C %x %x", &a, &b) == 2) { rpm = (uint16_t)(((a<<8)|b)/4); return true; }
  return false;
}

static bool parse_pid_010D(const char* line, float& kmh) {
  const char* p = strstr(line, "41 0D");
  if (!p) return false;
  unsigned v; if (sscanf(p, "41 0D %x", &v) == 1) { kmh = (float)v; return true; }
  return false;
}

static bool parse_pid_0111(const char* line, float& thr) {
  const char* p = strstr(line, "41 11");
  if (!p) return false;
  unsigned v; if (sscanf(p, "41 11 %x", &v) == 1) { thr = (float)v * 100.0f / 255.0f; return true; }
  return false;
}

void obd2_tick() {
  if (!bt_ready) return;
  if (!SerialBT.connected()) return; // only when connected as client

  // Simple periodic polling sequence every 200 ms for three PIDs
  char line[128];
  uint32_t now = millis();
  if (now - last_query_ms > 200) {
    last_query_ms = now;
    send_cmd("010C"); // RPM
    if (readline(line, sizeof line)) {
      uint16_t rpm; if (parse_pid_010C(line, rpm)) { g_vehicle_data.rpm = rpm; g_vehicle_data.last_ms = now; g_vehicle_data.obd_ok = true; }
    }
    send_cmd("010D"); // Speed
    if (readline(line, sizeof line)) {
      float kmh; if (parse_pid_010D(line, kmh)) { g_vehicle_data.speed_mps = kmh * 0.2777778f; g_vehicle_data.last_ms = now; g_vehicle_data.obd_ok = true; }
    }
    send_cmd("0111"); // Throttle
    if (readline(line, sizeof line)) {
      float thr; if (parse_pid_0111(line, thr)) { g_vehicle_data.throttle = thr; g_vehicle_data.last_ms = now; g_vehicle_data.obd_ok = true; }
    }
  }
}
