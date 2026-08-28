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

    const char* timezone() const;
    bool use24Hour() const;
    void setClockSettings(const String& timezone, bool use24Hour);

    const char* weatherLatitude() const;
    const char* weatherLongitude() const;
    void setWeatherLocation(const String& latitude, const String& longitude);

private:
    Preferences preferences_;
    AppMode defaultMode_ = AppMode::Face;
    uint8_t batteryPercent_ = 100;
    unsigned long lastBatterySampleAt_ = 0;
    bool batterySampled_ = false;
    char wifiSsid_[65] = {};
    char wifiPassword_[65] = {};
    char timezone_[40] = {};
    bool use24Hour_ = true;
    char latitude_[16] = {};
    char longitude_[16] = {};
};
