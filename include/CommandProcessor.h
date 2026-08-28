#pragma once

#include <Arduino.h>
#include "AppController.h"
#include "MotorController.h"
#include "WiFiDiagnostics.h"

// One parser behind every control surface. Serial, BLE and the web API all
// call execute(), so a command can never mean two different things depending
// on where it was typed. Output goes to a Print sink rather than straight to
// Serial, which is what lets the same code answer a BLE notify or an HTTP body.
class CommandProcessor {
public:
    CommandProcessor(AppController& app, MotorController& motors);

    // Returns false for an unrecognised command, having written nothing to
    // `out`; callers map that onto their own error convention (a 400, say).
    bool execute(const String& line, Print& out);

    // Last radio check, shared so the OLED screen and the JSON endpoint report
    // the same numbers the console printed.
    const WiFiDiagnostics& lastDiagnostics() const;
    void setLastDiagnostics(const WiFiDiagnostics& diag);

private:
    AppController& app_;
    MotorController& motors_;
    WiFiDiagnostics lastDiagnostics_;

    bool applyMode(const String& command, Print& out);
    bool applyEmotion(const String& command, unsigned long now, Print& out);
    bool applyMotor(const String& verb, const String& argument, Print& out);
    void printStatus(Print& out);
    void printHelp(Print& out);
    void runDiagnostics(Print& out);
};
