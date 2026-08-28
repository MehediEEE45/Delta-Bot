#include "SerialConsole.h"

SerialConsole::SerialConsole(CommandProcessor& processor)
    : processor_(processor) {}

void SerialConsole::begin() {
    Serial.println();
    Serial.println("Console ready. Type 'help' for commands.");
    Serial.print("> ");
}

void SerialConsole::dispatch() {
    buffer_[length_] = '\0';
    const String line(buffer_);
    length_ = 0;

    if (overflowed_) {
        overflowed_ = false;
        Serial.println("line too long, ignored");
    } else if (line.length() > 0) {
        if (!processor_.execute(line, Serial)) {
            Serial.print("unknown command: ");
            Serial.println(line);
        }
    }
    Serial.print("> ");
}

void SerialConsole::update() {
    while (Serial.available() > 0) {
        const char incoming = static_cast<char>(Serial.read());
        if (incoming == '\r') continue;
        if (incoming == '\n') {
            dispatch();
            continue;
        }
        if (length_ + 1 >= BufferSize) {
            overflowed_ = true;
            continue;
        }
        buffer_[length_++] = incoming;
    }
}
