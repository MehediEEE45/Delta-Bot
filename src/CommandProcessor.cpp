#include "CommandProcessor.h"
#include <WiFi.h>
#include "Config.h"

namespace {
struct EmotionName {
    const char* name;
    Emotion emotion;
    unsigned long durationMs;
};

// Surprised is deliberately timed; every other expression latches until
// something else replaces it.
constexpr EmotionName Emotions[] = {
    {"happy", Emotion::Happy, 0},
    {"love", Emotion::Love, 0},
    {"angry", Emotion::Angry, 0},
    {"cool", Emotion::Cool, 0},
    {"excited", Emotion::Excited, 0},
    {"sad", Emotion::Sad, 0},
    {"surprised", Emotion::Surprised, 5000},
    {"sleep", Emotion::Sleep, 0},
    {"idle", Emotion::Idle, 0},
};
constexpr int EmotionCount = sizeof(Emotions) / sizeof(Emotions[0]);
}

CommandProcessor::CommandProcessor(AppController& app, MotorController& motors)
    : app_(app), motors_(motors) {}

const WiFiDiagnostics& CommandProcessor::lastDiagnostics() const { return lastDiagnostics_; }

void CommandProcessor::setLastDiagnostics(const WiFiDiagnostics& diag) { lastDiagnostics_ = diag; }

bool CommandProcessor::applyMode(const String& command, Print& out) {
    AppMode mode;
    if (!appModeFromName(command, mode)) return false;
    app_.setMode(mode);
    out.print("mode -> ");
    out.println(app_.modeName());
    return true;
}

bool CommandProcessor::applyEmotion(const String& command, unsigned long now, Print& out) {
    for (int i = 0; i < EmotionCount; ++i) {
        if (command != Emotions[i].name) continue;
        app_.setEmotion(Emotions[i].emotion, Emotions[i].durationMs, now);
        out.print("emotion -> ");
        out.println(app_.emotionName());
        return true;
    }
    return false;
}

bool CommandProcessor::applyMotor(const String& verb, const String& argument, Print& out) {
    MotorCommand command;
    if (verb == "forward") command = MotorCommand::Forward;
    else if (verb == "backward") command = MotorCommand::Backward;
    else if (verb == "left") command = MotorCommand::Left;
    else if (verb == "right") command = MotorCommand::Right;
    else if (verb == "stop") command = MotorCommand::Stop;
    else return false;

    // A bare drive command from a terminal has no slider behind it, so default
    // to a speed that moves the bot without launching it off the desk.
    const int speed = argument.isEmpty() ? 180 : constrain(argument.toInt(), 0, 255);
    motors_.drive(command, static_cast<uint8_t>(command == MotorCommand::Stop ? 0 : speed));
    out.print("motor -> ");
    out.println(motors_.commandName());
    return true;
}

void CommandProcessor::printStatus(Print& out) {
    out.print("mode    : ");
    out.println(app_.modeName());
    out.print("emotion : ");
    out.println(app_.emotionName());
    out.print("motor   : ");
    out.println(motors_.commandName());
    out.print("battery : ");
    if (Config::BATTERY_ADC_PIN == 255) {
        out.println("not measured (no divider fitted)");
    } else {
        out.print(app_.batteryPercent());
        out.println('%');
    }
    out.print("night   : ");
    out.println(app_.nightActive() ? "yes" : "no");
    out.print("wifi    : ");
    if (WiFi.status() == WL_CONNECTED) {
        out.print("online, ");
        out.print(WiFi.localIP().toString());
        out.print(", rssi ");
        out.print(WiFi.RSSI());
        out.println(" dBm");
    } else if (WiFi.getMode() == WIFI_AP) {
        out.print("access point, ");
        out.println(WiFi.softAPIP().toString());
    } else {
        out.println("offline");
    }
    out.print("uptime  : ");
    out.print(millis() / 1000);
    out.println("s");
}

void CommandProcessor::printHelp(Print& out) {
    out.println("commands:");
    out.println("  status              current mode, radio, battery, uptime");
    out.println("  wifidiag            re-run the radio check and report");
    out.println("  wifi                alias for wifidiag");
    out.println("  <mode>              face, clock, weather, tasks, pomodoro,");
    out.println("                      pet, quotes, music, canvas, guard, night");
    out.println("  <emotion>           happy, love, angry, cool, excited, sad,");
    out.println("                      surprised, sleep, idle");
    out.println("  drive <dir> [speed] forward|backward|left|right|stop, 0-255");
    out.println("  restart             reboot the board");
    out.println("  help                this list");
}

void CommandProcessor::runDiagnostics(Print& out) {
    out.println("running radio check, this takes a few seconds...");
    // Re-running the live check rather than replaying the boot capture is the
    // point: it answers "is the radio working *now*" without a power cycle.
    const WiFiDiagnostics diag = runWiFiDiagnostics(app_.wifiSsid(), app_.wifiPassword(), 15000, nullptr);
    lastDiagnostics_ = diag;
    diag.report(out);
}

bool CommandProcessor::execute(const String& line, Print& out) {
    String command = line;
    command.trim();
    command.toLowerCase();
    if (command.isEmpty()) return false;

    String verb = command;
    String argument;
    const int space = command.indexOf(' ');
    if (space >= 0) {
        verb = command.substring(0, space);
        argument = command.substring(space + 1);
        argument.trim();
    }

    if (verb == "help" || verb == "?") { printHelp(out); return true; }
    if (verb == "status") { printStatus(out); return true; }
    if (verb == "wifidiag" || verb == "wifi") { runDiagnostics(out); return true; }
    if (verb == "drive") {
        String direction = argument;
        String speed;
        const int split = argument.indexOf(' ');
        if (split >= 0) {
            direction = argument.substring(0, split);
            speed = argument.substring(split + 1);
            speed.trim();
        }
        return applyMotor(direction, speed, out);
    }
    if (verb == "restart" || verb == "reboot") {
        out.println("restarting...");
        out.flush();
        delay(100);
        ESP.restart();
        return true;
    }

    const unsigned long now = millis();
    if (applyMode(command, out)) return true;
    if (applyEmotion(command, now, out)) return true;
    if (applyMotor(verb, argument, out)) return true;

    return false;
}
