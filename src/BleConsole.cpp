#include "BleConsole.h"
#include "Config.h"

namespace {
// Nordic UART Service. These exact UUIDs are what off-the-shelf BLE terminal
// apps look for, which is why the bot needs no companion app of its own.
constexpr char NusServiceUuid[] = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char NusRxUuid[] = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
constexpr char NusTxUuid[] = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

// A Print sink that notifies back over the TX characteristic, so the shared
// CommandProcessor writes its output to BLE exactly as it does to Serial.
class BlePrint : public Print {
public:
    BlePrint(NimBLECharacteristic* characteristic, bool connected)
        : characteristic_(characteristic), connected_(connected) {}

    size_t write(uint8_t value) override {
        if (!characteristic_ || !connected_) return 0;
        buffer_ += static_cast<char>(value);
        // Default BLE MTU carries 20 bytes of payload; flush on a full chunk
        // or at a line end so the client sees output as it is produced.
        if (value == '\n' || buffer_.length() >= 20) flush();
        return 1;
    }

    void flush() override {
        if (!characteristic_ || !connected_ || buffer_.isEmpty()) return;
        characteristic_->setValue(reinterpret_cast<const uint8_t*>(buffer_.c_str()), buffer_.length());
        characteristic_->notify();
        buffer_ = "";
        // The stack needs a moment between notifies or later ones are dropped.
        delay(12);
    }

private:
    NimBLECharacteristic* characteristic_;
    bool connected_;
    String buffer_;
};
}

BleConsole::BleConsole(CommandProcessor& processor)
    : processor_(processor) {}

void BleConsole::ServerCallbacks::onConnect(NimBLEServer* server, NimBLEConnInfo&) {
    owner_.connected_ = true;
    Serial.println("BLE console connected.");
    (void)server;
}

void BleConsole::ServerCallbacks::onDisconnect(NimBLEServer* server, NimBLEConnInfo&, int) {
    owner_.connected_ = false;
    Serial.println("BLE console disconnected.");
    // Without this the bot is invisible after the first client drops.
    if (server) NimBLEDevice::startAdvertising();
}

void BleConsole::RxCallbacks::onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo&) {
    if (!characteristic) return;
    owner_.handleIncoming(characteristic->getValue());
}

void BleConsole::enqueue(const char* text) {
    if (!queue_) return;
    Line line;
    strncpy(line.text, text, LineSize - 1);
    line.text[LineSize - 1] = '\0';
    // Non-blocking: a full queue means the main loop is behind, and dropping
    // the newest command is better than stalling the BLE stack's task.
    xQueueSend(queue_, &line, 0);
}

void BleConsole::handleIncoming(const std::string& chunk) {
    for (const char incoming : chunk) {
        if (incoming == '\r') continue;
        if (incoming == '\n') {
            pending_[pendingLength_] = '\0';
            if (pendingLength_ > 0) enqueue(pending_);
            pendingLength_ = 0;
            continue;
        }
        if (pendingLength_ + 1 >= LineSize) {
            pendingLength_ = 0;
            continue;
        }
        pending_[pendingLength_++] = incoming;
    }
    // Terminal apps commonly send a line with no terminator at all, so treat
    // the end of a write as an end of line when something is buffered.
    if (pendingLength_ > 0) {
        pending_[pendingLength_] = '\0';
        enqueue(pending_);
        pendingLength_ = 0;
    }
}

bool BleConsole::begin() {
    queue_ = xQueueCreate(QueueDepth, sizeof(Line));
    if (!queue_) {
        Serial.println("BLE console: queue allocation failed; BLE disabled.");
        return false;
    }

    NimBLEDevice::init(Config::DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(&serverCallbacks_);

    NimBLEService* service = server->createService(NusServiceUuid);
    txCharacteristic_ = service->createCharacteristic(NusTxUuid, NIMBLE_PROPERTY::NOTIFY);
    NimBLECharacteristic* rx = service->createCharacteristic(
        NusRxUuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    rx->setCallbacks(&rxCallbacks_);
    service->start();

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(NusServiceUuid);
    advertising->setName(Config::DEVICE_NAME);
    advertising->start();

    started_ = true;
    Serial.print("BLE console advertising as: ");
    Serial.println(Config::DEVICE_NAME);
    return true;
}

bool BleConsole::connected() const { return connected_; }

void BleConsole::update() {
    if (!started_ || !queue_) return;
    Line line;
    while (xQueueReceive(queue_, &line, 0) == pdTRUE) {
        BlePrint out(txCharacteristic_, connected_);
        if (!processor_.execute(String(line.text), out)) {
            out.print("unknown command: ");
            out.println(line.text);
        }
        out.flush();
    }
}
