#pragma once

#include <Arduino.h>
#include "Secrets.h"

namespace Config {
constexpr uint8_t I2C_SDA_PIN = 8;
constexpr uint8_t I2C_SCL_PIN = 9;
constexpr uint8_t TOUCH_PIN = 4;
constexpr uint8_t MOTOR_LEFT_PWM = 3;
constexpr uint8_t MOTOR_LEFT_IN1 = 5;
constexpr uint8_t MOTOR_LEFT_IN2 = 6;
constexpr uint8_t MOTOR_RIGHT_PWM = 0;
constexpr uint8_t MOTOR_RIGHT_IN1 = 7;
constexpr uint8_t MOTOR_RIGHT_IN2 = 10;
constexpr uint8_t MOTOR_STBY = 1;
constexpr bool TOUCH_ACTIVE_HIGH = true;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint8_t BATTERY_ADC_PIN = 255;
constexpr float BATTERY_DIVIDER_RATIO = 2.0f;
constexpr float BATTERY_EMPTY_VOLTAGE = 3.30f;
constexpr float BATTERY_FULL_VOLTAGE = 4.20f;
constexpr char FALLBACK_AP_SSID[] = "Delta";
constexpr char TIMEZONE[] = "CET-1CEST,M3.5.0,M10.5.0";

constexpr char WEATHER_LATITUDE[] = "52.5200";
constexpr char WEATHER_LONGITUDE[] = "13.4050";

constexpr unsigned long TOUCH_DEBOUNCE_MS = 50;
constexpr unsigned long GESTURE_WINDOW_MS = 250;
constexpr unsigned long LONG_PRESS_MS = 1500;
constexpr unsigned long WEATHER_REFRESH_MS = 900000;
constexpr unsigned long WEATHER_RETRY_MS = 60000;
constexpr unsigned long WEATHER_HTTP_TIMEOUT_MS = 8000;
constexpr unsigned long WEATHER_TLS_HANDSHAKE_TIMEOUT_S = 10;
constexpr unsigned long IDLE_SLEEP_MS = 600000;
constexpr unsigned long IDLE_SURPRISE_INTERVAL_MS = 30000;
constexpr unsigned long IDLE_SURPRISE_DURATION_MS = 2000;
constexpr unsigned long STATUS_REFRESH_MS = 1000;

// Battery sampling: average this many reads, no more often than this interval.
constexpr uint8_t BATTERY_SAMPLE_COUNT = 8;
constexpr unsigned long BATTERY_SAMPLE_INTERVAL_MS = 5000;

// Pomodoro Timer Constants
constexpr unsigned long POMODORO_WORK_MS = 25 * 60 * 1000UL;
constexpr unsigned long POMODORO_BREAK_MS = 5 * 60 * 1000UL;

// Virtual Pet Constants
constexpr unsigned long PET_DECAY_INTERVAL_MS = 60000UL; // decay stats every minute

// NVS is only rewritten this often for background state (pet decay, battery),
// so idle decay cannot burn through the flash. Explicit user edits save at once.
constexpr unsigned long FLASH_THROTTLE_MS = 300000UL; // 5 minutes

// Night Mode Hours
constexpr int NIGHT_START_HOUR = 22; // 10 PM
constexpr int NIGHT_END_HOUR = 6;    // 6 AM

// Canvas Size
constexpr uint8_t CANVAS_WIDTH = 128;
constexpr uint8_t CANVAS_HEIGHT = 64;
constexpr uint8_t CANVAS_CELLS_X = 16;
constexpr uint8_t CANVAS_CELLS_Y = 16;
constexpr size_t CANVAS_CELL_COUNT = CANVAS_CELLS_X * CANVAS_CELLS_Y;
// Canvas buffer is stored in Adafruit_GFX drawBitmap layout: row-major,
// MSB-first, CANVAS_WIDTH/8 bytes per row.
constexpr size_t CANVAS_BUFFER_BYTES = (CANVAS_WIDTH / 8) * CANVAS_HEIGHT;

// Frame pacing. A full 1KB I2C transfer at 400kHz already costs ~25ms, so
// anything faster than this starves the web server and touch sampling.
constexpr unsigned long MUSIC_FRAME_MS = 50;
constexpr unsigned long CLOCK_FRAME_MS = 200;

// Motors coast to a stop if the browser stops sending drive commands.
constexpr unsigned long MOTOR_WATCHDOG_MS = 1000;
constexpr uint8_t MOTOR_SHAKE_SPEED = 150;
constexpr unsigned long MOTOR_SHAKE_LEG_MS = 150;
}
