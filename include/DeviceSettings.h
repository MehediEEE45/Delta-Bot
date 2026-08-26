#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "FaceRenderer.h"

class DeviceSettings {
public:
    void begin();
    AppMode defaultMode() const;
    uint8_t batteryPercent() const;
    bool batteryMeasured() const;
    void updateBattery(unsigned long now);
    void setDefaultMode(AppMode mode);
    const char* wifiSsid() const;
    const char* wifiPassword() const;
    void setWiFiCredentials(const String& ssid, const String& password);

private:
    Preferences preferences_;
    AppMode defaultMode_ = AppMode::TimeDate;
    uint8_t batteryPercent_ = 100;
    unsigned long lastBatterySampleAt_ = 0;
    bool batterySampled_ = false;
    char wifiSsid_[65] = {};
    char wifiPassword_[65] = {};
};
