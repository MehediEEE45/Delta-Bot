#include "AppMode.h"

const char* appModeName(AppMode mode) {
    switch (mode) {
        case AppMode::TimeDate:  return "time";
        case AppMode::Weather:   return "weather";
        case AppMode::Music:     return "music";
        case AppMode::Tasks:     return "tasks";
        case AppMode::Notice:    return "notice";
        case AppMode::Reminder:  return "reminder";
        case AppMode::Pomodoro:  return "pomodoro";
        case AppMode::Canvas:    return "canvas";
        case AppMode::Quotes:    return "quotes";
        case AppMode::DeskGuard: return "guard";
        case AppMode::Pet:       return "pet";
        case AppMode::Decision:  return "decision";
        case AppMode::Night:     return "night";
        case AppMode::RCCar:     return "rc_car";
        default:                 return "time";
    }
}

bool appModeFromName(const String& name, AppMode& out) {
    if (name == "clock") { out = AppMode::TimeDate; return true; }
    for (uint8_t raw = 0; raw < static_cast<uint8_t>(AppMode::Count); ++raw) {
        const AppMode mode = static_cast<AppMode>(raw);
        if (name == appModeName(mode)) {
            out = mode;
            return true;
        }
    }
    return false;
}

bool appModeIsValid(uint8_t raw) {
    return raw < static_cast<uint8_t>(AppMode::Count);
}
