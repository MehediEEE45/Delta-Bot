#pragma once

#include "FaceRenderer.h"

class ClockMode {
public:
    void update(AppMode mode, unsigned long now, Emotion emotion, FaceRenderer& renderer, const WeatherData& weather, bool wifiOnline);

private:
    unsigned long lastFrameAt_ = 0;
};