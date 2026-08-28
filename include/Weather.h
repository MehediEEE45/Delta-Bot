#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

struct WeatherData {
    float temperature = NAN;
    int weatherCode = -1;
    bool valid = false;
    unsigned long updatedAt = 0;
    bool requesting = false;
    bool lastRequestFailed = false;
};

// Fetches current conditions from Open-Meteo on a dedicated FreeRTOS task, so
// the TLS handshake never blocks the render loop or the web server.
class WeatherService {
public:
    void begin();
    void update(unsigned long now);
    void requestRefresh();
    // Only ever call this from the main task. The worker only reads it inside
    // fetch(), which only runs after a task notification -- the same
    // happens-before edge fetchInFlight_ already relies on, so no lock needed.
    void setLocation(const String& latitude, const String& longitude);
    const WeatherData& data() const;
    bool isOnline() const;
    const char* statusName() const;

private:
    static void taskEntry(void* arg);
    void taskLoop();
    bool fetch(WeatherData& out);

    WeatherData data_;   // only ever touched by the main loop
    // The worker hands results over through a length-1 queue rather than a
    // shared struct: FreeRTOS copies the payload inside a critical section, so
    // there is no ordering assumption between the data and a "ready" flag.
    QueueHandle_t resultQueue_ = nullptr;
    volatile bool fetchInFlight_ = false;
    bool online_ = false;
    unsigned long lastAttemptAt_ = 0;
    TaskHandle_t task_ = nullptr;
    char latitude_[16] = "23.8103";
    char longitude_[16] = "90.4125";
};
