#pragma once

#include "AppMode.h"
#include "ClockMode.h"
#include "DeviceSettings.h"
#include "MusicMode.h"
#include "TaskManager.h"
#include "TouchSensor.h"
#include "Weather.h"

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
    AppMode defaultMode() const;
    const char* defaultModeName() const;
    uint8_t batteryPercent() const;
    const char* wifiSsid() const;
    const char* wifiPassword() const;
    void setWiFiCredentials(const String& ssid, const String& password);
    const char* modeName() const;
    const char* emotionName() const;
    bool nightActive() const;

    TaskManager& taskManager();

private:
    FaceRenderer& renderer_;
    WeatherService& weather_;
    DeviceSettings settings_;
    MusicMode musicMode_;
    ClockMode clockMode_;
    TaskManager taskManager_;

    AppMode mode_ = AppMode::TimeDate;
    Emotion emotion_ = Emotion::Idle;
    unsigned long emotionUntil_ = 0;
    bool emotionTimed_ = false;
    unsigned long lastInteractionAt_ = 0;
    unsigned long nextIdleEmotionAt_ = 0;

    // Night mode is applied on the 22:00 / 06:00 edges only, so it can never
    // fight a mode the user picked while it is dark.
    bool nightActive_ = false;
    bool nightInitialised_ = false;
    AppMode preNightMode_ = AppMode::TimeDate;

    void checkNightMode();
    void checkReminder(unsigned long now);
    void noteInteraction(unsigned long now);
    Emotion restingEmotion() const;
};
