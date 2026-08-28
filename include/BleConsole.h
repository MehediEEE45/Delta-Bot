#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "CommandProcessor.h"

// Same console as the serial one, over a Nordic UART Service so any generic
// BLE terminal app can drive the bot with no custom client.
//
// NimBLE runs its callbacks on the stack's own task. Rather than touch
// AppController from there, the write callback only posts the line to a queue;
// update() drains it on the main task, so every command still executes from
// exactly one thread.
class BleConsole {
public:
    explicit BleConsole(CommandProcessor& processor);
    bool begin();
    void update();
    bool connected() const;

private:
    static constexpr size_t LineSize = 96;
    static constexpr size_t QueueDepth = 4;

    struct Line {
        char text[LineSize];
    };

    class RxCallbacks : public NimBLECharacteristicCallbacks {
    public:
        explicit RxCallbacks(BleConsole& owner) : owner_(owner) {}
        void onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo& info) override;
    private:
        BleConsole& owner_;
    };

    class ServerCallbacks : public NimBLEServerCallbacks {
    public:
        explicit ServerCallbacks(BleConsole& owner) : owner_(owner) {}
        void onConnect(NimBLEServer* server, NimBLEConnInfo& info) override;
        void onDisconnect(NimBLEServer* server, NimBLEConnInfo& info, int reason) override;
    private:
        BleConsole& owner_;
    };

    CommandProcessor& processor_;
    NimBLECharacteristic* txCharacteristic_ = nullptr;
    QueueHandle_t queue_ = nullptr;
    RxCallbacks rxCallbacks_{*this};
    ServerCallbacks serverCallbacks_{*this};
    volatile bool connected_ = false;
    bool started_ = false;
    // Assembles across writes, since a BLE client may split a line over
    // several packets or send several lines in one.
    char pending_[LineSize] = {};
    size_t pendingLength_ = 0;

    void enqueue(const char* text);
    void handleIncoming(const std::string& chunk);
};
