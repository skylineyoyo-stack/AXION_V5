// AXION V4 — Global declarations
#pragma once

#include <Arduino.h>

// Forward enums
enum class UIMode : uint8_t;

// Global state
// ISR flags (naming per plan)
extern volatile bool g_flag_pps;
extern volatile bool g_flag_sqw;
extern volatile bool g_flag_int_imu;

// ISR timestamps (micros at edge)
extern volatile uint32_t g_ts_pps_us;
extern volatile uint32_t g_ts_sqw_us;
extern UIMode g_mode;

// System orchestrators
void axion_setup();
void axion_loop();

// Utilities
void i2c_scan();

// PCF8574 & I/O
void pcf_begin();
void pcf_set_bit(uint8_t bit, bool high);
bool pcf_read_bit(uint8_t bit);
bool btn_up();
bool btn_down();
bool btn_select();
void led_set(bool red_on, bool green_on);
void oled_cs(bool active);
void oled_reset_pulse();

// OLED
void oled_begin();
void oled_fill(uint8_t gray4bit);
void oled_clear();
void oled_blit();
void oled_draw_text(int x, int y, const char* text, uint8_t gray = 0xF);
void oled_draw_value(int x, int y, const char* label, float value, uint8_t gray = 0xF);
void oled_draw_pixel(int x, int y, uint8_t gray = 0xF);
void oled_fill_rect(int x, int y, int w, int h, uint8_t gray = 0xF);
void oled_sleep(bool off);
// LGFX-friendly helpers
void oled_set_font_small();
void oled_set_font_medium();
void oled_set_font_large();
void oled_draw_string(int x, int y, const char* text, uint8_t gray = 0xF);
void oled_fill_circle(int x, int y, int r, uint8_t gray = 0xF);

// Buzzer
void buzzer_begin();
void buzzer_beep(uint16_t freq, uint16_t ms, uint8_t duty = 128);

// GPS
void gps_begin();
void gps_tick();
float gps_course_deg();

// RTC
void rtc_begin();
void rtc_sync_from_gps(uint32_t unix_epoch);
uint32_t rtc_get_unix();

// IMU
void imu_begin();
void imu_tick();
bool imu_mag_calibrated();

// OBD2
void obd2_begin();
void obd2_tick();
bool obd2_is_connected();
bool obd2_connect_saved();
bool obd2_connect_by_name(const char* name);
bool obd2_start_pairing();
bool obd2_scan_devices_start();
bool obd2_scan_in_progress();
size_t obd2_scan_count();
bool obd2_scan_get(size_t idx, char* name_out, size_t name_max, uint8_t mac_out[6]);
bool obd2_connect_index(size_t idx);
bool obd2_forget_saved();

// Logger & Power
void logger_init();
void logger_tick();
// Session logging (SD) — optional
void session_start_drag();
void session_stop_drag();
bool session_drag_active();
void session_start_drift();
void session_stop_drift();
bool session_drift_active();
void session_discard_last();
void power_init();
void power_tick();
void power_note_activity();
void power_request_deep_sleep();
void logger_flush();

// UI
void ui_render_current();
void ui_draw_topbar(const char* mode_name);
void ui_transition_to(UIMode newMode, bool forward);
void toast_show(const char* msg, uint32_t duration_ms);
void toast_draw_if_any();

// Off-screen sprites for slide transitions (LGFX only; no-ops otherwise)
void oled_sprite_begin_A();
void oled_sprite_begin_B();
void oled_sprite_end();
void oled_sprite_push_A(int x, int y);
void oled_sprite_push_B(int x, int y);

// Fusion
void speed_fusion_tick();
float fused_speed_mps();
uint8_t speed_confidence_pct();
void drift_update();
bool accel_run_active();

// Menu
void menu_init();
void menu_update();

// Lap manager (persisted track/ghost)
bool lap_load_from_eeprom();
bool lap_save_to_eeprom();
void lap_set_slot(int slot);
int  lap_get_slot();

// Error logger
void error_log_init();
void error_log(uint8_t code, const char* source, int32_t data);
void error_log_dump(uint16_t last_n);
bool error_log_persist(uint16_t max_to_save);
uint16_t error_log_count();

// System persistent state
struct SystemState;
void system_state_note_boot();
void system_state_set_last_uptime(uint32_t ms);

// Bus locks
void bus_locks_init();
void bus_i2c_lock();
void bus_i2c_unlock();
void bus_spi_lock();
void bus_spi_unlock();
