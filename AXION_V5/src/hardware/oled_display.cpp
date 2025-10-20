// AXION V4 — OLED SSD1322 minimal driver (fast path)

#include <Arduino.h>
#include <SPI.h>
#include <string.h>
#include <pgmspace.h>
#include "hardware/pins.h"
#include "axion.h"
#ifdef USE_LOVYANGFX
// Fallback to internal driver path even when LGFX flag is set, to ensure
// compatibility with SSD1322. Implement software sprites for transitions.

static SPIClass spi(VSPI);

// Framebuffers: main + two sprites
static uint8_t FB[256*64/2];
static uint8_t FB_A[256*64/2];
static uint8_t FB_B[256*64/2];
static uint8_t s_target = 0; // 0=FB, 1=FB_A, 2=FB_B

static inline uint8_t* curbuf() { return s_target==1?FB_A:(s_target==2?FB_B:FB); }

static inline void fb_clear(uint8_t g) {
  uint8_t v = (uint8_t)((g << 4) | (g & 0x0F));
  memset(curbuf(), v, 256*64/2);
}

static inline void fb_draw_pixel_buf(uint8_t* buf, int x, int y, uint8_t g) {
  if (x < 0 || x >= 256 || y < 0 || y >= 64) return;
  size_t idx = (size_t)y * 128 + (x >> 1);
  if ((x & 1) == 0) buf[idx] = (uint8_t)((g << 4) | (buf[idx] & 0x0F));
  else              buf[idx] = (uint8_t)((buf[idx] & 0xF0) | (g & 0x0F));
}

static inline void fb_draw_pixel(int x, int y, uint8_t g) { fb_draw_pixel_buf(curbuf(), x, y, g); }

static void fb_fill_rect(int x, int y, int w, int h, uint8_t g) {
  if (w <= 0 || h <= 0) return;
  int x1 = x + w - 1; int y1 = y + h - 1;
  if (x < 0) x = 0; if (y < 0) y = 0; if (x1 > 255) x1 = 255; if (y1 > 63) y1 = 63;
  for (int yy = y; yy <= y1; ++yy)
    for (int xx = x; xx <= x1; ++xx)
      fb_draw_pixel(xx, yy, g);
}

// 5x7 font and string drawer (ASCII 32..90)
static const uint8_t FONT5x7[] PROGMEM = {
  0,0,0,0,0,  0,0,95,0,0,  0,7,0,7,0,  20,127,20,127,20,  36,42,127,42,18, 35,19,8,100,98,  54,73,85,34,80,  0,5,3,0,0,
  0,28,34,65,0,  0,65,34,28,0,  20,8,62,8,20,  8,8,62,8,8,  0,80,48,0,0,  8,8,8,8,8,  0,96,96,0,0,  32,16,8,4,2,
  62,81,73,69,62,  0,66,127,64,0,  66,97,81,73,70,  33,65,69,75,49,  24,20,18,127,16,  39,69,69,69,57,  60,74,73,73,48,  1,113,9,5,3,  54,73,73,73,54,  6,73,73,41,30,
  0,54,54,0,0,  0,86,54,0,0,  8,20,34,65,0,  20,20,20,20,20,  0,65,34,20,8,  2,1,81,9,6,
  126,17,17,17,126,  127,73,73,73,54,  62,65,65,65,34,  127,65,65,34,28,  127,73,73,73,65,  127,9,9,9,1,  62,65,81,81,114,  127,8,8,8,127,
  0,65,127,65,0,  32,64,65,63,1,  127,8,20,34,65,  127,64,64,64,64,  127,2,12,2,127,  127,4,8,16,127,  62,65,65,65,62,  127,9,9,9,6,
  62,65,81,33,94,  127,9,25,41,70,  38,73,73,73,50,  1,1,127,1,1,  63,64,64,64,63,  31,32,64,32,31,  63,64,56,64,63,  99,20,8,20,99,  3,4,120,4,3,  97,81,73,69,67
};

static void fb_draw_char(int x, int y, char c, uint8_t g) {
  if (c < 32 || c > 90) c = '?';
  int idx = (c - 32) * 5;
  for (int col = 0; col < 5; ++col) {
    uint8_t bits = pgm_read_byte(&FONT5x7[idx + col]);
    for (int row = 0; row < 7; ++row) {
      if (bits & (1 << row)) fb_draw_pixel(x + col, y + row, g);
    }
  }
}

static void fb_draw_text(int x, int y, const char* s, uint8_t g) {
  int cx = x; while (*s) { if (*s=='\n'){ y+=8; cx=x; ++s; continue; } fb_draw_char(cx,y,*s,g); cx+=6; ++s; }
}

static inline void oled_cmd(uint8_t c) {
  digitalWrite(PIN_OLED_DC, LOW);
  oled_cs(true);
  spi.transfer(c);
  oled_cs(false);
}

static inline void oled_data_bytes(const uint8_t* data, size_t len) {
  digitalWrite(PIN_OLED_DC, HIGH);
  oled_cs(true);
  spi.transfer((void*)data, len);
  oled_cs(false);
}

void oled_begin() {
  pinMode(PIN_OLED_DC, OUTPUT);
  digitalWrite(PIN_OLED_DC, LOW);
  spi.begin(PIN_SPI_SCLK, -1, PIN_SPI_MOSI, -1);
  spi.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));
  oled_reset_pulse();
  oled_cmd(0xFD); uint8_t unlock = 0x12; oled_data_bytes(&unlock, 1);
  oled_cmd(0xAE);
  oled_cmd(0xB3); uint8_t clk = 0x91; oled_data_bytes(&clk, 1);
  oled_cmd(0xCA); uint8_t mux = 0x3F; oled_data_bytes(&mux, 1);
  oled_cmd(0xA2); uint8_t offs = 0x00; oled_data_bytes(&offs, 1);
  oled_cmd(0xA1); uint8_t startline = 0x00; oled_data_bytes(&startline, 1);
  oled_cmd(0xA0); uint8_t remap[] = { 0x14, 0x11 }; oled_data_bytes(remap, 2);
  oled_cmd(0xB5); uint8_t gpio = 0x00; oled_data_bytes(&gpio, 1);
  oled_cmd(0xAB); uint8_t func = 0x01; oled_data_bytes(&func, 1);
  oled_cmd(0xB4); uint8_t enh[] = { 0xA0, 0xFD }; oled_data_bytes(enh, 2);
  oled_cmd(0xC1); uint8_t contrast = 0x9F; oled_data_bytes(&contrast, 1);
  oled_cmd(0xC7); uint8_t gray = 0x0F; oled_data_bytes(&gray, 1);
  oled_cmd(0xB1); uint8_t phase = 0xE2; oled_data_bytes(&phase, 1);
  oled_cmd(0xD1); uint8_t enhC[] = { 0x82, 0x20 }; oled_data_bytes(enhC, 2);
  oled_cmd(0xBB); uint8_t prechg = 0x1F; oled_data_bytes(&prechg, 1);
  oled_cmd(0xB6); uint8_t prechg2 = 0x08; oled_data_bytes(&prechg2, 1);
  oled_cmd(0xBE); uint8_t vcomh = 0x07; oled_data_bytes(&vcomh, 1);
  oled_cmd(0xA6);
  oled_cmd(0x15); uint8_t col[] = { 0x1C, 0x5B }; oled_data_bytes(col, 2);
  oled_cmd(0x75); uint8_t row[] = { 0x00, 0x3F }; oled_data_bytes(row, 2);
  oled_cmd(0xAF);
  oled_fill(0x0);
}

void oled_fill(uint8_t gray4bit) {
  fb_clear(gray4bit & 0x0F);
  oled_cmd(0x15); uint8_t col[] = { 0x1C, 0x5B }; oled_data_bytes(col, 2);
  oled_cmd(0x75); uint8_t row[] = { 0x00, 0x3F }; oled_data_bytes(row, 2);
  oled_cmd(0x5C);
  uint8_t* buf = curbuf();
  for (int i = 0; i < 256*64/2; i += 128) oled_data_bytes(buf + i, 128);
}

void oled_clear() { fb_clear(0x0); }

void oled_blit() {
  oled_cmd(0x15); uint8_t col[] = { 0x1C, 0x5B }; oled_data_bytes(col, 2);
  oled_cmd(0x75); uint8_t row[] = { 0x00, 0x3F }; oled_data_bytes(row, 2);
  oled_cmd(0x5C);
  uint8_t* buf = curbuf();
  for (int i = 0; i < 256*64/2; i += 128) oled_data_bytes(buf + i, 128);
}

void oled_draw_text(int x, int y, const char* text, uint8_t gray) { fb_draw_text(x, y, text, (uint8_t)(gray & 0x0F)); }
void oled_draw_value(int x, int y, const char* label, float value, uint8_t gray) { char b[48]; snprintf(b,sizeof b, "%s: %.2f", label?label:"", value); oled_draw_text(x,y,b,gray); }
void oled_draw_pixel(int x, int y, uint8_t gray) { fb_draw_pixel(x, y, (uint8_t)(gray & 0x0F)); }
void oled_fill_rect(int x, int y, int w, int h, uint8_t gray) { fb_fill_rect(x, y, w, h, (uint8_t)(gray & 0x0F)); }
void oled_sleep(bool off) { if (off) oled_cmd(0xAE); else oled_cmd(0xAF); }
void oled_set_font_small() {}
void oled_set_font_medium() {}
void oled_set_font_large() {}
void oled_draw_string(int x, int y, const char* text, uint8_t gray) { oled_draw_text(x,y,text,gray); }
void oled_fill_circle(int x, int y, int r, uint8_t gray) {
  int rr=r*r; uint8_t g=(uint8_t)(gray&0x0F); for (int yy=-r; yy<=r; ++yy){ int w=(int)sqrtf((float)(rr-yy*yy)); fb_fill_rect(x-w, y+yy, w*2+1, 1, g);} }

// Software sprites: draw into FB_A/FB_B then compose into FB by horizontal shifts
void oled_sprite_begin_A() { s_target=1; memset(FB_A,0,sizeof(FB_A)); }
void oled_sprite_begin_B() { s_target=2; memset(FB_B,0,sizeof(FB_B)); }
void oled_sprite_end()     { s_target=0; }

static void compose_shift(uint8_t* dst, const uint8_t* src, int shift_px) {
  if (shift_px & 1) shift_px -= (shift_px>0?1:-1);
  int byte_shift = shift_px/2;
  for (int y=0;y<64;++y){
    uint8_t* d = dst + y*128; const uint8_t* s = src + y*128;
    if (byte_shift>=0){ int cnt=128-byte_shift; if (cnt>0) memcpy(d+byte_shift, s, (size_t)cnt); }
    else { int off=-byte_shift; int cnt=128-off; if (cnt>0) memcpy(d, s+off, (size_t)cnt); }
  }
}

void oled_sprite_push_A(int x, int /*y*/) { memset(FB,0,sizeof(FB)); compose_shift(FB, FB_A, x); }
void oled_sprite_push_B(int x, int /*y*/) {
  uint8_t tmp[sizeof(FB)]; memset(tmp,0,sizeof tmp);
  compose_shift(tmp, FB_B, x);
  for (size_t i=0;i<sizeof(FB);++i){ uint8_t s=tmp[i]; if (s){ uint8_t d=FB[i]; uint8_t hi=(s&0xF0)?(s&0xF0):(d&0xF0); uint8_t lo=(s&0x0F)?(s&0x0F):(d&0x0F); FB[i]=hi|lo; } }
}

#else

static SPIClass spi(VSPI);

// Simple 4-bit framebuffer: 256x64, 2 pixels per byte (high nibble = x even)
static uint8_t FB[256*64/2];

static inline void fb_clear(uint8_t g) {
  uint8_t v = (uint8_t)((g << 4) | (g & 0x0F));
  memset(FB, v, sizeof(FB));
}

static inline void fb_draw_pixel(int x, int y, uint8_t g) {
  if (x < 0 || x >= 256 || y < 0 || y >= 64) return;
  size_t idx = (size_t)y * 128 + (x >> 1);
  if ((x & 1) == 0) {
    FB[idx] = (uint8_t)((g << 4) | (FB[idx] & 0x0F));
  } else {
    FB[idx] = (uint8_t)((FB[idx] & 0xF0) | (g & 0x0F));
  }
}

static void fb_fill_rect(int x, int y, int w, int h, uint8_t g) {
  if (w <= 0 || h <= 0) return;
  int x1 = x + w - 1;
  int y1 = y + h - 1;
  if (x < 0) x = 0; if (y < 0) y = 0;
  if (x1 > 255) x1 = 255; if (y1 > 63) y1 = 63;
  for (int yy = y; yy <= y1; ++yy) {
    for (int xx = x; xx <= x1; ++xx) fb_draw_pixel(xx, yy, g);
  }
}

// Minimal 5x7 font for ASCII 32..90 (subset) — each row is 5 bits wide
static const uint8_t FONT5x7[] PROGMEM = {
  // 32..47 (space to /)
  0,0,0,0,0,  0,0,95,0,0,  0,7,0,7,0,  20,127,20,127,20,  36,42,127,42,18, 35,19,8,100,98,  54,73,85,34,80,  0,5,3,0,0,
  0,28,34,65,0,  0,65,34,28,0,  20,8,62,8,20,  8,8,62,8,8,  0,80,48,0,0,  8,8,8,8,8,  0,96,96,0,0,  32,16,8,4,2,
  // 48..57 (0-9)
  62,81,73,69,62,  0,66,127,64,0,  66,97,81,73,70,  33,65,69,75,49,  24,20,18,127,16,  39,69,69,69,57,  60,74,73,73,48,  1,113,9,5,3,  54,73,73,73,54,  6,73,73,41,30,
  // 58..64 (: to @)
  0,54,54,0,0,  0,86,54,0,0,  8,20,34,65,0,  20,20,20,20,20,  0,65,34,20,8,  2,1,81,9,6,
  // 65..90 (A-Z)
  126,17,17,17,126,  127,73,73,73,54,  62,65,65,65,34,  127,65,65,34,28,  127,73,73,73,65,  127,9,9,9,1,  62,65,81,81,114,  127,8,8,8,127,
  0,65,127,65,0,  32,64,65,63,1,  127,8,20,34,65,  127,64,64,64,64,  127,2,12,2,127,  127,4,8,16,127,  62,65,65,65,62,  127,9,9,9,6,
  62,65,81,33,94,  127,9,25,41,70,  38,73,73,73,50,  1,1,127,1,1,  63,64,64,64,63,  31,32,64,32,31,  63,64,56,64,63,  99,20,8,20,99,  3,4,120,4,3,  97,81,73,69,67
};

static void fb_draw_char(int x, int y, char c, uint8_t g) {
  if (c < 32 || c > 90) c = '?';
  int idx = (c - 32) * 5;
  for (int col = 0; col < 5; ++col) {
    uint8_t bits = pgm_read_byte(&FONT5x7[idx + col]);
    for (int row = 0; row < 7; ++row) {
      if (bits & (1 << row)) fb_draw_pixel(x + col, y + row, g);
    }
  }
}

static void fb_draw_text(int x, int y, const char* s, uint8_t g) {
  int cx = x;
  while (*s) {
    if (*s == '\n') { y += 8; cx = x; ++s; continue; }
    fb_draw_char(cx, y, *s, g);
    cx += 6;
    ++s;
  }
}

static inline void oled_cmd(uint8_t c) {
  digitalWrite(PIN_OLED_DC, LOW);
  oled_cs(true);
  spi.transfer(c);
  oled_cs(false);
}

static inline void oled_data_bytes(const uint8_t* data, size_t len) {
  digitalWrite(PIN_OLED_DC, HIGH);
  oled_cs(true);
  spi.transfer((void*)data, len);
  oled_cs(false);
}

void oled_begin() {
  pinMode(PIN_OLED_DC, OUTPUT);
  digitalWrite(PIN_OLED_DC, LOW);

  spi.begin(PIN_SPI_SCLK, -1, PIN_SPI_MOSI, -1);
  spi.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));

  oled_reset_pulse();

  // Init sequence (SSD1322 datasheet based)
  oled_cmd(0xFD); uint8_t unlock = 0x12; oled_data_bytes(&unlock, 1);
  oled_cmd(0xAE);
  oled_cmd(0xB3); uint8_t clk = 0x91; oled_data_bytes(&clk, 1);
  oled_cmd(0xCA); uint8_t mux = 0x3F; oled_data_bytes(&mux, 1);
  oled_cmd(0xA2); uint8_t offs = 0x00; oled_data_bytes(&offs, 1);
  oled_cmd(0xA1); uint8_t startline = 0x00; oled_data_bytes(&startline, 1);
  oled_cmd(0xA0); uint8_t remap[] = { 0x14, 0x11 }; oled_data_bytes(remap, 2);
  oled_cmd(0xB5); uint8_t gpio = 0x00; oled_data_bytes(&gpio, 1);
  oled_cmd(0xAB); uint8_t func = 0x01; oled_data_bytes(&func, 1);
  oled_cmd(0xB4); uint8_t enh[] = { 0xA0, 0xFD }; oled_data_bytes(enh, 2);
  oled_cmd(0xC1); uint8_t contrast = 0x9F; oled_data_bytes(&contrast, 1);
  oled_cmd(0xC7); uint8_t gray = 0x0F; oled_data_bytes(&gray, 1);
  oled_cmd(0xB1); uint8_t phase = 0xE2; oled_data_bytes(&phase, 1);
  oled_cmd(0xD1); uint8_t enhC[] = { 0x82, 0x20 }; oled_data_bytes(enhC, 2);
  oled_cmd(0xBB); uint8_t prechg = 0x1F; oled_data_bytes(&prechg, 1);
  oled_cmd(0xB6); uint8_t prechg2 = 0x08; oled_data_bytes(&prechg2, 1);
  oled_cmd(0xBE); uint8_t vcomh = 0x07; oled_data_bytes(&vcomh, 1);
  oled_cmd(0xA6);

  // Full area window (256x64)
  oled_cmd(0x15); uint8_t col[] = { 0x1C, 0x5B }; oled_data_bytes(col, 2);
  oled_cmd(0x75); uint8_t row[] = { 0x00, 0x3F }; oled_data_bytes(row, 2);

  oled_cmd(0xAF);

  // Clear
  oled_fill(0x0);
}

void oled_fill(uint8_t gray4bit) {
  // Fill framebuffer and blit full-screen
  fb_clear(gray4bit & 0x0F);

  // Set window
  oled_cmd(0x15); uint8_t col[] = { 0x1C, 0x5B }; oled_data_bytes(col, 2);
  oled_cmd(0x75); uint8_t row[] = { 0x00, 0x3F }; oled_data_bytes(row, 2);
  oled_cmd(0x5C);

  // Push full buffer
  for (int i = 0; i < (int)sizeof(FB); i += 128) {
    oled_data_bytes(&FB[i], 128);
  }
}

void oled_clear() { fb_clear(0x0); }

void oled_blit() {
  // Full-area write as in oled_fill
  oled_cmd(0x15); uint8_t col[] = { 0x1C, 0x5B }; oled_data_bytes(col, 2);
  oled_cmd(0x75); uint8_t row[] = { 0x00, 0x3F }; oled_data_bytes(row, 2);
  oled_cmd(0x5C);
  for (int i = 0; i < (int)sizeof(FB); i += 128) {
    oled_data_bytes(&FB[i], 128);
  }
}

void oled_draw_text(int x, int y, const char* text, uint8_t gray) {
  fb_draw_text(x, y, text, (uint8_t)(gray & 0x0F));
}

void oled_draw_value(int x, int y, const char* label, float value, uint8_t gray) {
  char buf[48];
  snprintf(buf, sizeof(buf), "%s: %.2f", label ? label : "", value);
  fb_draw_text(x, y, buf, (uint8_t)(gray & 0x0F));
}

void oled_draw_pixel(int x, int y, uint8_t gray) {
  fb_draw_pixel(x, y, (uint8_t)(gray & 0x0F));
}

void oled_fill_rect(int x, int y, int w, int h, uint8_t gray) {
  fb_fill_rect(x, y, w, h, (uint8_t)(gray & 0x0F));
}

void oled_sleep(bool off) {
  if (off) {
    // Display off
    oled_cmd(0xAE);
  } else {
    // Display on
    oled_cmd(0xAF);
  }
}

void oled_set_font_small() {}
void oled_set_font_medium() {}
void oled_set_font_large() {}
void oled_draw_string(int x, int y, const char* text, uint8_t gray) { oled_draw_text(x,y,text,gray); }
void oled_fill_circle(int x, int y, int r, uint8_t gray) {
  // Midpoint circle fill (approx)
  int rr = r*r;
  for (int yy=-r; yy<=r; ++yy) {
    int w = (int)sqrtf((float)(rr - yy*yy));
    oled_fill_rect(x - w, y + yy, w*2+1, 1, gray);
  }
}
// No-op sprite helpers in non-LGFX mode
void oled_sprite_begin_A() {}
void oled_sprite_begin_B() {}
void oled_sprite_end() {}
void oled_sprite_push_A(int, int) {}
void oled_sprite_push_B(int, int) {}

#endif // USE_LOVYANGFX
