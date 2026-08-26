#include "ClockMode.h"
#include "Config.h"

void ClockMode::update(AppMode mode, unsigned long now, Emotion emotion, FaceRenderer& renderer, const WeatherData& weather, bool wifiOnline, TaskManager* taskMgr) {
    const bool animated = mode == AppMode::Face || mode == AppMode::RCCar;
    const unsigned long frameTimeMs = animated ? Config::FACE_FRAME_MS : Config::CLOCK_FRAME_MS;
    if (lastFrameAt_ != 0 && now - lastFrameAt_ < frameTimeMs) return;
    lastFrameAt_ = now;
    renderer.render(mode, emotion, weather, wifiOnline, now, taskMgr);
}