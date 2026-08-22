#include "DeviceSettings.h"
#include "Config.h"
#include <cstring>

void DeviceSettings::begin() {
    preferences_.begin("delta", false);
    const uint8_t savedMode = preferences_.getUChar("mode", 1);
    defaultMode_ = savedMode == 0 ? AppMode::Music : (savedMode == 2 ? AppMode::Weather : AppMode::TimeDate);
    batteryPercent_ = preferences_.getUChar("battery", 95);
    String savedSsid = preferences_.getString("ssid", Config::WIFI_SSID);
    String savedPassword = preferences_.getString("password", Config::WIFI_PASSWORD);
    savedSsid.toCharArray(wifiSsid_, sizeof(wifiSsid_));
    savedPassword.toCharArray(wifiPassword_, sizeof(wifiPassword_));
}

AppMode DeviceSettings::defaultMode() const { return defaultMode_; }
uint8_t DeviceSettings::batteryPercent() const { return batteryPercent_; }

void DeviceSettings::updateBattery() {
    if (Config::BATTERY_ADC_PIN == 255) return;
    const int raw = analogRead(Config::BATTERY_ADC_PIN);
    const float voltage = (raw / 4095.0f) * 3.3f * Config::BATTERY_DIVIDER_RATIO;
    const float percent = ((voltage - Config::BATTERY_EMPTY_VOLTAGE) * 100.0f) /
        (Config::BATTERY_FULL_VOLTAGE - Config::BATTERY_EMPTY_VOLTAGE);
    batteryPercent_ = static_cast<uint8_t>(constrain(percent, 0.0f, 100.0f));
}

void DeviceSettings::setDefaultMode(AppMode mode) {
    defaultMode_ = mode;
    preferences_.putUChar("mode", mode == AppMode::Music ? 0 : (mode == AppMode::Weather ? 2 : 1));
}

const char* DeviceSettings::wifiSsid() const { return wifiSsid_; }
const char* DeviceSettings::wifiPassword() const { return wifiPassword_; }

void DeviceSettings::setWiFiCredentials(const String& ssid, const String& password) {
    ssid.toCharArray(wifiSsid_, sizeof(wifiSsid_));
    password.toCharArray(wifiPassword_, sizeof(wifiPassword_));
    preferences_.putString("ssid", wifiSsid_);
    preferences_.putString("password", wifiPassword_);
}
