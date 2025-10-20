// AXION V4 — GPS NEO-8M (UART2 + PPS)

#include <Arduino.h>
#include <ctype.h>
#include <string.h>
#include "hardware/pins.h"
#include "sensor_structs.h"

static HardwareSerial GPS(2);

// Simple NMEA line buffer
static char nmea[128];
static int nidx = 0;

static uint8_t hex2(uint8_t c) {
  if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
  c = (uint8_t)toupper(c);
  if (c >= 'A' && c <= 'F') return (uint8_t)(10 + c - 'A');
  return 0;
}

static bool nmea_checksum_ok(const char* s) {
  // s points to '$', find '*' and compute xor of chars between
  const char* p = s;
  if (*p != '$') return false;
  ++p; uint8_t cs = 0;
  while (*p && *p != '*') { cs ^= (uint8_t)(*p); ++p; }
  if (*p != '*') return false;
  if (!isxdigit((unsigned char)p[1]) || !isxdigit((unsigned char)p[2])) return false;
  uint8_t got = (uint8_t)((hex2((uint8_t)p[1]) << 4) | hex2((uint8_t)p[2]));
  return cs == got;
}

static double nmea_latlon_to_deg(const char* fld, char hemi) {
  if (!fld || !*fld) return 0.0;
  double v = atof(fld);
  int deg = (int)(v / 100);
  double minutes = v - deg * 100;
  double degs = deg + minutes / 60.0;
  if (hemi == 'S' || hemi == 'W') degs = -degs;
  return degs;
}

static void parse_gprmc(char* s) {
  // $GPRMC,time,status,lat,N,lon,E,speed(kn),course,date,...*CS
  // Tokenize by comma
  char* f[16] = {0}; int nf = 0;
  for (char* p = s; *p && nf < 16; ++p) {
    if (*p == ',') { *p = '\0'; f[nf++] = p + 1; }
  }
  if (nf < 11) return;
  char status = f[1] && *f[1] ? *f[1] : 'V';
  if (status == 'A') {
    double lat = nmea_latlon_to_deg(f[2], f[3] ? *f[3] : 'N');
    double lon = nmea_latlon_to_deg(f[4], f[5] ? *f[5] : 'E');
    float spd_kn = f[6] ? (float)atof(f[6]) : 0.0f;
    float course = f[7] ? (float)atof(f[7]) : 0.0f;
    g_gps_data.lat = lat;
    g_gps_data.lon = lon;
    g_gps_data.speed_mps = spd_kn * 0.514444f;
    g_gps_data.heading_deg = course;
    g_gps_data.lock = true;
    g_gps_data.last_fix_ms = millis();
  } else {
    g_gps_data.lock = false;
  }
}

static void parse_gpgga(char* s) {
  // $GPGGA,time,lat,N,lon,E,fix,sats,hdop,alt, ... *CS
  char* f[16] = {0}; int nf = 0;
  for (char* p = s; *p && nf < 16; ++p) {
    if (*p == ',') { *p = '\0'; f[nf++] = p + 1; }
  }
  if (nf < 10) return;
  int fix = f[5] ? atoi(f[5]) : 0;
  uint8_t sats = f[6] ? (uint8_t)atoi(f[6]) : 0;
  float hdop = f[7] ? (float)atof(f[7]) : 0.0f;
  float alt = f[8] ? (float)atof(f[8]) : 0.0f;
  if (fix > 0) {
    g_gps_data.sats = sats;
    g_gps_data.hdop = hdop;
    g_gps_data.alt = alt;
  }
}

void gps_begin() {
  GPS.begin(115200, SERIAL_8N1, PIN_GPS_RX2, PIN_GPS_TX2);
}

void gps_tick() {
  while (GPS.available()) {
    char c = (char)GPS.read();
    if (c == '\r') continue;
    if (c == '\n') {
      nmea[nidx] = '\0';
      if (nidx > 6 && nmea[0] == '$' && nmea_checksum_ok(nmea)) {
        // Strip checksum for parsing (replace * with \0)
        char* star = strchr(nmea, '*'); if (star) *star = '\0';
        if (strstr(nmea, "$GPRMC") == nmea) parse_gprmc(nmea);
        else if (strstr(nmea, "$GNRMC") == nmea) parse_gprmc(nmea);
        else if (strstr(nmea, "$GPGGA") == nmea || strstr(nmea, "$GNGGA") == nmea) parse_gpgga(nmea);
      }
      nidx = 0;
    } else {
      if (nidx < (int)sizeof(nmea) - 1) nmea[nidx++] = c; else nidx = 0;
    }
  }
}

float gps_course_deg() { return g_gps_data.heading_deg; }
