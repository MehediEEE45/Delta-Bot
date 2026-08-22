#include "ClockMode.h"

void ClockMode::update(AppMode mode, unsigned long now, Emotion emotion, FaceRenderer& renderer, const WeatherData& weather, bool wifiOnline) {
    constexpr unsigned long frameTimeMs = 200;
    if (lastFrameAt_ != 0 && now - lastFrameAt_ < frameTimeMs) return;
    lastFrameAt_ = now;
    renderer.render(mode, emotion, weather, wifiOnline, now);
}