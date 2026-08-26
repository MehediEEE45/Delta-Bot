#pragma once

#include <Arduino.h>

enum class AppMode {
    TimeDate,
    Weather,
    Music,
    Tasks,
    Notice,
    Reminder,
    Pomodoro,
    Canvas,
    Quotes,
    DeskGuard,
    Pet,
    Decision,
    Night,
    RCCar,
    Count // sentinel; keep last
};

// Canonical string name for a mode. Used by the status API, the persisted
// default-mode setting, and the command parser so they cannot drift apart.
const char* appModeName(AppMode mode);

// Parses a canonical name (plus a few aliases) back into a mode.
bool appModeFromName(const String& name, AppMode& out);

// True when the raw value is a real mode and not the Count sentinel.
bool appModeIsValid(uint8_t raw);
