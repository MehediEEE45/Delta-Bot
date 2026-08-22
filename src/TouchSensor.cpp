#include "TouchSensor.h"
#include "Config.h"

TouchSensor::TouchSensor(uint8_t pin) : pin_(pin) {}

const char* touchEventName(TouchEvent event) {
    switch (event) {
        case TouchEvent::SingleTap: return "single_tap";
        case TouchEvent::DoubleTap: return "double_tap";
        case TouchEvent::TripleTap: return "triple_tap";
        case TouchEvent::LongPress: return "long_press";
        default: return "none";
    }
}

void TouchSensor::begin() {
    pinMode(pin_, INPUT);
    const bool active = digitalRead(pin_) == (Config::TOUCH_ACTIVE_HIGH ? HIGH : LOW);
    stableState_ = active;
    lastReading_ = active;
    lastChangeAt_ = millis();
    if (active) pressedAt_ = lastChangeAt_;
}

TouchEvent TouchSensor::update(unsigned long now) {
    const bool reading = digitalRead(pin_) == (Config::TOUCH_ACTIVE_HIGH ? HIGH : LOW);
    if (reading != lastReading_) {
        lastReading_ = reading;
        lastChangeAt_ = now;
    }

    if (now - lastChangeAt_ < Config::TOUCH_DEBOUNCE_MS) {
        return TouchEvent::None;
    }

    if (reading != stableState_) {
        stableState_ = reading;
        if (stableState_) {
            pressedAt_ = now;
            longPressSent_ = false;
        } else if (!longPressSent_) {
            tapCount_ = min<uint8_t>(tapCount_ + 1, 3);
            lastTapAt_ = now;
        }
    }

    if (stableState_ && !longPressSent_ && now - pressedAt_ >= Config::LONG_PRESS_MS) {
        longPressSent_ = true;
        tapCount_ = 0;
        return TouchEvent::LongPress;
    }

    if (!stableState_ && tapCount_ > 0 && now - lastTapAt_ >= Config::GESTURE_WINDOW_MS) {
        const uint8_t count = tapCount_;
        tapCount_ = 0;
        if (count == 1) return TouchEvent::SingleTap;
        if (count == 2) return TouchEvent::DoubleTap;
        return TouchEvent::TripleTap;
    }

    return TouchEvent::None;
}