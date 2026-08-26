#include "ClockMode.h"
#include "Config.h"

void ClockMode::update(AppMode mode, unsigned long now, Emotion emotion, FaceRenderer& renderer, const WeatherData& weather, bool wifiOnline, TaskManager* taskMgr) {
    const unsigned long frameTimeMs = Config::CLOCK_FRAME_MS;
    if (lastFrameAt_ != 0 && now - lastFrameAt_ < frameTimeMs) return;
    lastFrameAt_ = now;
    renderer.render(mode, emotion, weather, wifiOnline, now, taskMgr);
}