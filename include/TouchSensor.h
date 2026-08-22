#pragma once

#include <Arduino.h>

enum class TouchEvent {
    None,
    SingleTap,
    DoubleTap,
    TripleTap,
    LongPress
};

class TouchSensor {
public:
    explicit TouchSensor(uint8_t pin);
    void begin();
    TouchEvent update(unsigned long now);

private:
    uint8_t pin_;
    bool stableState_ = false;
    bool lastReading_ = false;
    bool longPressSent_ = false;
    unsigned long lastChangeAt_ = 0;
    unsigned long pressedAt_ = 0;
    unsigned long lastTapAt_ = 0;
    uint8_t tapCount_ = 0;
};

const char* touchEventName(TouchEvent event);