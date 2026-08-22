#include "MusicMode.h"

void MusicMode::update(unsigned long now, Emotion emotion, FaceRenderer& renderer, const WeatherData& weather, bool wifiOnline) {
    constexpr unsigned long frameTimeMs = 25;
    if (lastFrameAt_ != 0 && now - lastFrameAt_ < frameTimeMs) return;
    lastFrameAt_ = now;
    renderer.render(AppMode::Music, emotion, weather, wifiOnline, now);
}