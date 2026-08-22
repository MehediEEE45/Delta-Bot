#pragma once

#include "ClockMode.h"
#include "DeviceSettings.h"
#include "MusicMode.h"
#include "TouchSensor.h"
#include "Weather.h"

class WebController;

class AppController {
public:
    AppController(FaceRenderer& renderer, WeatherService& weather);
    void begin();
    void update(unsigned long now);
    void handleTouch(TouchEvent event, unsigned long now);
    void setMode(AppMode mode);
    void setEmotion(Emotion emotion, unsigned long durationMs, unsigned long now);
    AppMode mode() const;
    Emotion emotion() const;
    const WeatherData& weatherData() const;
    void requestWeatherRefresh();
    void setDefaultMode(AppMode mode);
    const char* defaultModeName() const;
    uint8_t batteryPercent() const;
    const char* wifiSsid() const;
    const char* wifiPassword() const;
    void setWiFiCredentials(const String& ssid, const String& password);
    const char* modeName() const;
    const char* emotionName() const;

private:
    FaceRenderer& renderer_;
    WeatherService& weather_;
    DeviceSettings settings_;
    MusicMode musicMode_;
    ClockMode clockMode_;
    AppMode mode_ = AppMode::TimeDate;
    Emotion emotion_ = Emotion::Idle;
    unsigned long emotionUntil_ = 0;
    unsigned long lastInteractionAt_ = 0;
    unsigned long nextIdleEmotionAt_ = 0;
};