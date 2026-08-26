#include "MusicMode.h"
#include "Config.h"

void MusicMode::update(unsigned long now, Emotion emotion, FaceRenderer& renderer, const WeatherData& weather, bool wifiOnline) {
    const unsigned long frameTimeMs = Config::MUSIC_FRAME_MS;
    if (lastFrameAt_ != 0 && now - lastFrameAt_ < frameTimeMs) return;
    lastFrameAt_ = now;
    renderer.render(AppMode::Music, emotion, weather, wifiOnline, now);
}