#pragma once

#include <Arduino.h>

struct WeatherData {
    float temperature = NAN;
    int weatherCode = -1;
    bool valid = false;
    unsigned long updatedAt = 0;
    bool requesting = false;
    bool lastRequestFailed = false;
};

class WeatherService {
public:
    void begin();
    void update(unsigned long now);
    void requestRefresh();
    const WeatherData& data() const;
    bool isOnline() const;
    const char* statusName() const;

private:
    WeatherData data_;
    bool online_ = false;
    unsigned long lastAttemptAt_ = 0;
};