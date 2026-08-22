#pragma once

#include <Arduino.h>
#include "Secrets.h"

namespace Config {
constexpr uint8_t I2C_SDA_PIN = 8;
constexpr uint8_t I2C_SCL_PIN = 9;
constexpr uint8_t TOUCH_PIN = 4 ;
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
constexpr char FALLBACK_AP_PASSWORD[] = "delta123";
constexpr char TIMEZONE[] = "CET-1CEST,M3.5.0,M10.5.0";

constexpr char WEATHER_LATITUDE[] = "52.5200";
constexpr char WEATHER_LONGITUDE[] = "13.4050";

constexpr unsigned long TOUCH_DEBOUNCE_MS = 50;
constexpr unsigned long GESTURE_WINDOW_MS = 250;
constexpr unsigned long LONG_PRESS_MS = 1500;
constexpr unsigned long WEATHER_REFRESH_MS = 900000;
constexpr unsigned long WEATHER_RETRY_MS = 60000;
constexpr unsigned long IDLE_SLEEP_MS = 600000;
constexpr unsigned long IDLE_SURPRISE_INTERVAL_MS = 30000;
constexpr unsigned long IDLE_SURPRISE_DURATION_MS = 2000;
constexpr unsigned long STATUS_REFRESH_MS = 1000;
}