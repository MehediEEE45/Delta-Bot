#pragma once

#include <Arduino.h>
#include "CommandProcessor.h"

// Line-buffered command reader on the USB serial port. Polled from loop(), so
// it never blocks waiting for a terminator the user may not send.
class SerialConsole {
public:
    explicit SerialConsole(CommandProcessor& processor);
    void begin();
    void update();

private:
    CommandProcessor& processor_;
    // Long enough for the longest real command; anything past it is a paste
    // accident, and the overflow is dropped rather than silently truncated
    // into a different valid command.
    static constexpr size_t BufferSize = 96;
    char buffer_[BufferSize] = {};
    size_t length_ = 0;
    bool overflowed_ = false;

    void dispatch();
};
