#pragma once

#include "FaceRenderer.h"

class MusicMode {
public:
    void update(unsigned long now, Emotion emotion, FaceRenderer& renderer, const WeatherData& weather, bool wifiOnline);

private:
    unsigned long lastFrameAt_ = 0;
};