#include "AppController.h"
#include "Config.h"
#include <WiFi.h>

AppController::AppController(FaceRenderer& renderer, WeatherService& weather)
    : renderer_(renderer), weather_(weather) {}

void AppController::begin() {
    settings_.begin();
    mode_ = settings_.defaultMode();
    emotion_ = mode_ == AppMode::Music ? Emotion::Happy : Emotion::Idle;
    lastInteractionAt_ = millis();
    nextIdleEmotionAt_ = lastInteractionAt_ + Config::IDLE_SURPRISE_INTERVAL_MS;
}

void AppController::update(unsigned long now) {
    settings_.updateBattery();
    if (mode_ == AppMode::Weather) {
        weather_.update(now);
    }
    if (emotion_ != Emotion::Sleep && now - lastInteractionAt_ >= Config::IDLE_SLEEP_MS) {
        emotion_ = Emotion::Sleep;
    }
    if (mode_ == AppMode::Music && emotion_ == Emotion::Happy && now >= nextIdleEmotionAt_) {
        emotion_ = Emotion::Surprised;
        emotionUntil_ = now + Config::IDLE_SURPRISE_DURATION_MS;
        nextIdleEmotionAt_ = now + Config::IDLE_SURPRISE_INTERVAL_MS;
    }
    if (emotionUntil_ != 0 && now >= emotionUntil_) {
        emotionUntil_ = 0;
        emotion_ = mode_ == AppMode::Music ? Emotion::Happy : Emotion::Idle;
    }
    if (mode_ == AppMode::Music) {
        musicMode_.update(now, emotion_, renderer_, weather_.data(), WiFi.status() == WL_CONNECTED);
    } else {
        clockMode_.update(mode_, now, emotion_, renderer_, weather_.data(), WiFi.status() == WL_CONNECTED);
    }
}

void AppController::handleTouch(TouchEvent event, unsigned long now) {
    if (event == TouchEvent::None) return;
    lastInteractionAt_ = now;
    nextIdleEmotionAt_ = now + Config::IDLE_SURPRISE_INTERVAL_MS;
    if (emotion_ == Emotion::Sleep && event != TouchEvent::LongPress) {
        emotion_ = mode_ == AppMode::Music ? Emotion::Happy : Emotion::Idle;
        emotionUntil_ = 0;
        return;
    }
    if (event == TouchEvent::SingleTap) {
        setEmotion(Emotion::Happy, 4000, now);
    } else if (event == TouchEvent::DoubleTap) {
        setEmotion(Emotion::Love, 5000, now);
    } else if (event == TouchEvent::TripleTap) {
        setEmotion(Emotion::Angry, 6000, now);
    } else if (event == TouchEvent::LongPress) {
        setEmotion(Emotion::Sleep, 0, now);
    }
}

void AppController::setMode(AppMode mode) {
    mode_ = mode;
    emotion_ = mode_ == AppMode::Music ? Emotion::Happy : Emotion::Idle;
    emotionUntil_ = 0;
}

void AppController::setEmotion(Emotion emotion, unsigned long durationMs, unsigned long now) {
    if (mode_ != AppMode::Music) {
        mode_ = AppMode::Music;
    }
    emotion_ = emotion;
    emotionUntil_ = durationMs == 0 ? 0 : now + durationMs;
}

AppMode AppController::mode() const { return mode_; }
Emotion AppController::emotion() const { return emotion_; }
const WeatherData& AppController::weatherData() const { return weather_.data(); }
void AppController::requestWeatherRefresh() { weather_.requestRefresh(); }
void AppController::setDefaultMode(AppMode mode) { settings_.setDefaultMode(mode); }
    const char* AppController::defaultModeName() const { return settings_.defaultMode() == AppMode::Music ? "music" : (settings_.defaultMode() == AppMode::Weather ? "weather" : "time"); }
uint8_t AppController::batteryPercent() const { return settings_.batteryPercent(); }
const char* AppController::wifiSsid() const { return settings_.wifiSsid(); }
const char* AppController::wifiPassword() const { return settings_.wifiPassword(); }
void AppController::setWiFiCredentials(const String& ssid, const String& password) { settings_.setWiFiCredentials(ssid, password); }

const char* AppController::modeName() const {
    if (mode_ == AppMode::Music) return "music";
    if (mode_ == AppMode::Weather) return "weather";
    return "time";
}

const char* AppController::emotionName() const {
    switch (emotion_) {
        case Emotion::Happy: return "happy";
        case Emotion::Love: return "love";
        case Emotion::Excited: return "excited";
        case Emotion::Cool: return "cool";
        case Emotion::Sad: return "sad";
        case Emotion::Angry: return "angry";
        case Emotion::Sleep: return "sleep";
        case Emotion::Surprised: return "surprised";
        default: return "idle";
    }
}