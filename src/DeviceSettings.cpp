#include "DeviceSettings.h"
#include "Config.h"
#include <cstring>

void DeviceSettings::begin() {
    preferences_.begin("delta", false);

    const uint8_t savedMode = preferences_.getUChar("mode", static_cast<uint8_t>(AppMode::TimeDate));
    defaultMode_ = appModeIsValid(savedMode) ? static_cast<AppMode>(savedMode) : AppMode::TimeDate;

    String savedSsid = preferences_.getString("ssid", Config::WIFI_SSID);
    String savedPassword = preferences_.getString("password", Config::WIFI_PASSWORD);
    savedSsid.toCharArray(wifiSsid_, sizeof(wifiSsid_));
    savedPassword.toCharArray(wifiPassword_, sizeof(wifiPassword_));
}

AppMode DeviceSettings::defaultMode() const { return defaultMode_; }
uint8_t DeviceSettings::batteryPercent() const { return batteryPercent_; }
bool DeviceSettings::batteryMeasured() const { return batterySampled_; }

void DeviceSettings::updateBattery(unsigned long now) {
    // No divider wired: report a full battery and flag it as unmeasured so the
    // API can say so rather than passing a constant off as a reading.
    if (Config::BATTERY_ADC_PIN == 255) return;
    if (batterySampled_ && now - lastBatterySampleAt_ < Config::BATTERY_SAMPLE_INTERVAL_MS) return;
    lastBatterySampleAt_ = now;

    // The ESP32 ADC is noisy and non-linear; average a burst of reads.
    uint32_t total = 0;
    for (uint8_t i = 0; i < Config::BATTERY_SAMPLE_COUNT; ++i) {
        total += analogReadMilliVolts(Config::BATTERY_ADC_PIN);
    }
    const float millivolts = static_cast<float>(total) / Config::BATTERY_SAMPLE_COUNT;
    const float voltage = (millivolts / 1000.0f) * Config::BATTERY_DIVIDER_RATIO;
    const float percent = ((voltage - Config::BATTERY_EMPTY_VOLTAGE) * 100.0f) /
        (Config::BATTERY_FULL_VOLTAGE - Config::BATTERY_EMPTY_VOLTAGE);

    batteryPercent_ = static_cast<uint8_t>(constrain(percent, 0.0f, 100.0f));
    batterySampled_ = true;
}

void DeviceSettings::setDefaultMode(AppMode mode) {
    defaultMode_ = mode;
    preferences_.putUChar("mode", static_cast<uint8_t>(mode));
}

const char* DeviceSettings::wifiSsid() const { return wifiSsid_; }
const char* DeviceSettings::wifiPassword() const { return wifiPassword_; }

void DeviceSettings::setWiFiCredentials(const String& ssid, const String& password) {
    ssid.toCharArray(wifiSsid_, sizeof(wifiSsid_));
    password.toCharArray(wifiPassword_, sizeof(wifiPassword_));
    preferences_.putString("ssid", wifiSsid_);
    preferences_.putString("password", wifiPassword_);
}
