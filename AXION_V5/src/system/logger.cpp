// AXION V4 — Logger CSV 10 Hz aligned to PPS

#include <Arduino.h>
#include "axion.h"
#include "ui_definitions.h"
#include "sensor_structs.h"
#include "system_config.h"
#ifdef USE_SD
#include "error_logger.h"
#endif
#ifdef USE_SD
#include <SD.h>
#include <SPI.h>
#include "hardware/pins.h"
static SPIClass sdSPI(HSPI);
static File logFile;
static uint32_t last_flush_ms = 0;
static bool sd_ok = false;
static void sd_open_file() {
  char name[32];
  uint32_t t = rtc_get_unix();
  // crude timestamp format
  snprintf(name, sizeof name, "/AXION_%lu.csv", (unsigned long)t);
  logFile = SD.open(name, FILE_WRITE);
}
#endif

static uint32_t seq = 0;
static uint32_t pps_idx = 0;
static uint32_t next_due_ms = 0;
#ifdef USE_SD
enum class SessType { None, Drag, Drift };
static SessType sess = SessType::None;
static File sessFile;
#ifndef NO_BIN_LOG
static File sessFileBin;
static uint32_t sessCRC = 0xFFFFFFFFu;
#endif
static struct { float gmax; float vmax; uint32_t lines; float drift_sum; uint32_t start_ms; char name[32]; } sessStats;
#endif

void logger_init() {
  seq = 0; pps_idx = 0; next_due_ms = 0;
  Serial.println("t_ms,idx_pps,rtc_s,lat,lon,alt,v_gps,azimuth,sats,HDOP,Gx,Gy,Gz,yaw,pitch,roll,v_obd,rpm,thr,v_fus,theta_drift,drift_score");
#ifdef USE_SD
  sdSPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  if (SD.begin(PIN_SD_CS, sdSPI)) { sd_ok = true; sd_open_file(); }
  sess = SessType::None; memset(&sessStats, 0, sizeof(sessStats));
#ifndef NO_BIN_LOG
  sessCRC = 0xFFFFFFFFu; sessFileBin = File();
#endif
#endif
}

static void write_csv_line() {
  uint32_t t = millis();
  uint32_t rtc = rtc_get_unix();
  float v_fus = fused_speed_mps();
  char line[256];
  int n = snprintf(line, sizeof line, "%lu,%lu,%lu,%.7f,%.7f,%.1f,%.2f,%.1f,%u,%.2f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%.2f,%u,%.0f,%.2f,%.2f\n",
                (unsigned long)t, (unsigned long)pps_idx, (unsigned long)rtc,
                g_gps_data.lat, g_gps_data.lon, g_gps_data.alt,
                g_gps_data.speed_mps, g_gps_data.heading_deg, g_gps_data.sats, g_gps_data.hdop,
                g_imu_data.ax, g_imu_data.ay, g_imu_data.az, g_imu_data.yaw, g_imu_data.pitch, g_imu_data.roll,
                g_vehicle_data.speed_mps, g_vehicle_data.rpm, g_vehicle_data.throttle,
                v_fus, g_drift_theta_deg, g_drift_score);
  Serial.write(line, n);
#ifdef USE_SD
  if (sd_ok && logFile) { logFile.write((const uint8_t*)line, n); }
  if (sd_ok && sessFile) {
    sessFile.write((const uint8_t*)line, n);
    sessStats.lines++;
    if (fabsf(g_imu_data.ax) > sessStats.gmax) sessStats.gmax=fabsf(g_imu_data.ax);
    if (g_gps_data.speed_mps>sessStats.vmax) sessStats.vmax=g_gps_data.speed_mps;
    sessStats.drift_sum += g_drift_score;
    #ifndef NO_BIN_LOG
    if (sessFileBin) {
      struct __attribute__((packed)) BinRec {
        uint32_t t_ms, idx_pps, rtc;
        float lat, lon, alt, v_gps, azimuth, sats_f, hdop;
        float ax, ay, az, yaw, pitch, roll, v_obd, rpm_f, thr, v_fus, theta, score;
      } r{};
      r.t_ms = t; r.idx_pps = pps_idx; r.rtc = rtc;
      r.lat = (float)g_gps_data.lat; r.lon = (float)g_gps_data.lon; r.alt = g_gps_data.alt; r.v_gps=g_gps_data.speed_mps; r.azimuth=g_gps_data.heading_deg; r.sats_f=(float)g_gps_data.sats; r.hdop=g_gps_data.hdop;
      r.ax=g_imu_data.ax; r.ay=g_imu_data.ay; r.az=g_imu_data.az; r.yaw=g_imu_data.yaw; r.pitch=g_imu_data.pitch; r.roll=g_imu_data.roll;
      r.v_obd=g_vehicle_data.speed_mps; r.rpm_f=(float)g_vehicle_data.rpm; r.thr=g_vehicle_data.throttle;
      r.v_fus=v_fus; r.theta=g_drift_theta_deg; r.score=g_drift_score;
      sessFileBin.write((const uint8_t*)&r, sizeof r);
      // CRC32 update
      auto crc32_update = [](uint32_t crc, const uint8_t* data, size_t len){
        crc = ~crc;
        while (len--) { crc ^= *data++; for (int i=0;i<8;++i) crc = (crc>>1) ^ (0xEDB88320u & (-(int)(crc & 1))); }
        return ~crc;
      };
      sessCRC = crc32_update(sessCRC, (const uint8_t*)&r, sizeof r);
    }
    #endif
  }
#endif
}

void logger_tick() {
  // Align to PPS: on PPS flag, set next_due to now (immediate) and increment index
  if (g_flag_pps) {
    pps_idx++;
    next_due_ms = millis();
    g_flag_pps = false;
  }
  // If next_due_ms not initialized, set now to start regular cadence
  if (next_due_ms == 0) next_due_ms = millis();

  if ((int32_t)(millis() - next_due_ms) >= 0) {
    write_csv_line();
    next_due_ms += 100; // 10 Hz
  }
#ifdef USE_SD
  if (sd_ok && logFile) {
    if (millis() - last_flush_ms > 10000) { logFile.flush(); last_flush_ms = millis(); }
  }
  // SD health check every 60s
  static uint32_t sd_last_check_ms = 0;
  static uint32_t sd_blink_until = 0;
  static uint32_t sd_blink_last = 0;
  static bool sd_blink_state = false;
  uint32_t now = millis();
  if (now < sd_blink_until && now - sd_blink_last > 200) { sd_blink_last = now; sd_blink_state = !sd_blink_state; led_set(sd_blink_state, false); }
  if (now - sd_last_check_ms >= 60000) {
    // Skip during active runs (Drag/Accel)
    if (session_drag_active() || accel_run_active()) {
      sd_last_check_ms = now; // postpone without testing
      return;
    }
    sd_last_check_ms = now;
    uint8_t bufw[1024]; for (size_t i=0;i<sizeof bufw;i++) bufw[i] = (uint8_t)(i ^ (now & 0xFF));
    auto crc32_update = [](uint32_t crc, const uint8_t* data, size_t len){
      crc = ~crc; while (len--) { crc ^= *data++; for (int i=0;i<8;++i) crc = (crc>>1) ^ (0xEDB88320u & (-(int)(crc & 1))); } return ~crc; };
    uint32_t c1 = crc32_update(0xFFFFFFFFu, bufw, sizeof bufw);
    bool ok = false;
    File f = SD.open("/SD_HEALTH.DAT", FILE_WRITE);
    if (f) { size_t w = f.write(bufw, sizeof bufw); f.flush(); f.close(); ok = (w == sizeof bufw); }
    if (ok) {
      File fr = SD.open("/SD_HEALTH.DAT", FILE_READ);
      if (fr) { uint8_t bufr[1024]; size_t r = fr.read(bufr, sizeof bufr); fr.close(); if (r == sizeof bufr) { uint32_t c2 = crc32_update(0xFFFFFFFFu, bufr, sizeof bufr); ok = (c1 == c2); } else ok = false; }
      else ok = false;
    }
    if (!ok) { error_log(ERR_SD_WRITE_FAIL, "SD", 0); sd_blink_until = now + 2000; sd_blink_last = now; sd_blink_state = false; }
  }
#endif
}

#ifdef USE_SD
static void open_session_file(const char* prefix) {
  if (!sd_ok) return;
  char name[32];
  uint32_t t = rtc_get_unix();
  snprintf(name, sizeof name, "/%s_%lu.csv", prefix, (unsigned long)t);
  sessFile = SD.open(name, FILE_WRITE);
  if (sessFile) {
    memset(&sessStats, 0, sizeof(sessStats));
    strncpy(sessStats.name, name, sizeof(sessStats.name)-1);
    sessStats.start_ms = millis();
    // Metadata header
    char meta[96];
    int m = snprintf(meta, sizeof meta, "# AXION v%u.%u\n# RTC %lu\n# SESSION %s\n",
                     AXION_MAJOR, AXION_MINOR, (unsigned long)rtc_get_unix(), prefix);
    sessFile.write((const uint8_t*)meta, m);
    const char* hdr = "t_ms,idx_pps,rtc_s,lat,lon,alt,v_gps,azimuth,sats,HDOP,Gx,Gy,Gz,yaw,pitch,roll,v_obd,rpm,thr,v_fus,theta_drift,drift_score\n";
    sessFile.write((const uint8_t*)hdr, strlen(hdr));
  }
  #ifndef NO_BIN_LOG
  char nameb[32]; snprintf(nameb, sizeof nameb, "/%s_%lu.bin", prefix, (unsigned long)t);
  sessFileBin = SD.open(nameb, FILE_WRITE);
  sessCRC = 0xFFFFFFFFu;
  if (sessFileBin) {
    struct __attribute__((packed)) BinHeader { uint32_t magic; uint16_t ver_major, ver_minor; uint32_t rtc; char session[8]; } h;
    h.magic = 0x4148424Eu; h.ver_major = AXION_MAJOR; h.ver_minor = AXION_MINOR; h.rtc = rtc_get_unix(); memset(h.session, 0, sizeof h.session); strncpy(h.session, prefix, sizeof(h.session)-1);
    sessFileBin.write((const uint8_t*)&h, sizeof h);
    auto crc32_update = [](uint32_t crc, const uint8_t* data, size_t len){
      crc = ~crc;
      while (len--) { crc ^= *data++; for (int i=0;i<8;++i) crc = (crc>>1) ^ (0xEDB88320u & (-(int)(crc & 1))); }
      return ~crc;
    };
    sessCRC = crc32_update(sessCRC, (const uint8_t*)&h, sizeof h);
  }
  #endif
}

void session_start_drag() { sess = SessType::Drag; open_session_file("RUN"); }
void session_stop_drag()  {
  if (sess==SessType::Drag && sessFile) {
    uint32_t dur = millis() - sessStats.start_ms;
    float drift_avg = (sessStats.lines>0) ? (sessStats.drift_sum / sessStats.lines) : 0.0f;
    char sum[128]; int n=snprintf(sum,sizeof sum,"# footer,lines,%lu,duration_ms,%lu,vmax,%.2f,gmax,%.2f,drift_avg,%.2f\n",
                                  (unsigned long)sessStats.lines, (unsigned long)dur, sessStats.vmax, sessStats.gmax, drift_avg);
    sessFile.write((const uint8_t*)sum,n); sessFile.flush(); sessFile.close();
    #ifndef NO_BIN_LOG
    if (sessFileBin) {
      struct __attribute__((packed)) BinFooter { uint32_t magic_end; uint32_t lines; uint32_t duration_ms; float vmax, gmax, drift_avg; uint32_t crc32; } f;
      f.magic_end = 0x424E4544u; f.lines = sessStats.lines; f.duration_ms = dur; f.vmax = sessStats.vmax; f.gmax = sessStats.gmax; f.drift_avg = drift_avg; f.crc32 = sessCRC;
      sessFileBin.write((const uint8_t*)&f, sizeof f); sessFileBin.flush(); sessFileBin.close();
    }
    #endif
  }
  sess=SessType::None;
}
bool session_drag_active() { return sess==SessType::Drag && sessFile; }

void session_start_drift(){ sess = SessType::Drift; open_session_file("DRIFT"); }
void session_stop_drift() {
  if (sess==SessType::Drift && sessFile) {
    uint32_t dur = millis() - sessStats.start_ms;
    float drift_avg = (sessStats.lines>0) ? (sessStats.drift_sum / sessStats.lines) : 0.0f;
    char sum[128]; int n=snprintf(sum,sizeof sum,"# footer,lines,%lu,duration_ms,%lu,vmax,%.2f,gmax,%.2f,drift_avg,%.2f\n",
                                  (unsigned long)sessStats.lines, (unsigned long)dur, sessStats.vmax, sessStats.gmax, drift_avg);
    sessFile.write((const uint8_t*)sum,n); sessFile.flush(); sessFile.close();
    #ifndef NO_BIN_LOG
    if (sessFileBin) {
      struct __attribute__((packed)) BinFooter { uint32_t magic_end; uint32_t lines; uint32_t duration_ms; float vmax, gmax, drift_avg; uint32_t crc32; } f;
      f.magic_end = 0x424E4544u; f.lines = sessStats.lines; f.duration_ms = dur; f.vmax = sessStats.vmax; f.gmax = sessStats.gmax; f.drift_avg = drift_avg; f.crc32 = sessCRC;
      sessFileBin.write((const uint8_t*)&f, sizeof f); sessFileBin.flush(); sessFileBin.close();
    }
    #endif
  }
  sess=SessType::None;
}
bool session_drift_active(){ return sess==SessType::Drift && sessFile; }
void session_discard_last(){ if (sd_ok && sessStats.name[0]) { SD.remove(sessStats.name); sessStats.name[0]=0; } }
#endif

void logger_flush() {
#ifdef USE_SD
  if (sd_ok && logFile) logFile.flush();
#endif
}
