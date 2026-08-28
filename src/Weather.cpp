#include "Weather.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "Config.h"

void WeatherService::begin() {
    lastAttemptAt_ = 0;
    resultQueue_ = xQueueCreate(1, sizeof(WeatherData));
    if (resultQueue_ == nullptr) {
        Serial.println("Weather: could not allocate result queue; disabled.");
        return;
    }
    // Priority 1 matches the Arduino loop task. The ESP32-C3 is single-core, so
    // the two round-robin; the loop keeps rendering through the TLS handshake
    // instead of stalling on it. Do not drop to 0 -- that is the idle priority.
    // 8KB of stack: mbedTLS needs a lot of room for the handshake.
    if (xTaskCreate(&WeatherService::taskEntry, "weather", 8192, this, 1, &task_) != pdPASS) {
        Serial.println("Weather: could not start worker task; disabled.");
        task_ = nullptr;
    }
}

void WeatherService::requestRefresh() {
    lastAttemptAt_ = 0;
}

void WeatherService::setLocation(const String& latitude, const String& longitude) {
    latitude.toCharArray(latitude_, sizeof(latitude_));
    longitude.toCharArray(longitude_, sizeof(longitude_));
}

void WeatherService::taskEntry(void* arg) {
    static_cast<WeatherService*>(arg)->taskLoop();
}

void WeatherService::taskLoop() {
    for (;;) {
        // Sleep until update() asks for work by raising fetchInFlight_.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        WeatherData result;
        fetch(result);
        xQueueOverwrite(resultQueue_, &result);
        fetchInFlight_ = false;
    }
}

bool WeatherService::fetch(WeatherData& out) {
    String url = "https://api.open-meteo.com/v1/forecast?latitude=";
    url += latitude_;
    url += "&longitude=";
    url += longitude_;
    url += "&current=temperature_2m,weather_code&timezone=auto";

    WiFiClientSecure client;
    client.setInsecure(); // Open-Meteo is public read-only data; no pinning needed.
    client.setHandshakeTimeout(Config::WEATHER_TLS_HANDSHAKE_TIMEOUT_S);

    HTTPClient http;
    http.setTimeout(Config::WEATHER_HTTP_TIMEOUT_MS);
    http.setConnectTimeout(Config::WEATHER_HTTP_TIMEOUT_MS);

    if (!http.begin(client, url)) {
        Serial.println("Weather: HTTPS connection setup failed");
        out.lastRequestFailed = true;
        return false;
    }

    const int result = http.GET();
    if (result != HTTP_CODE_OK) {
        Serial.print("Weather: HTTP error ");
        Serial.println(result);
        out.lastRequestFailed = true;
        http.end();
        return false;
    }

    JsonDocument document;
    const DeserializationError error = deserializeJson(document, http.getStream());
    http.end();

    if (error) {
        Serial.print("Weather: JSON error: ");
        Serial.println(error.c_str());
        out.lastRequestFailed = true;
        return false;
    }

    out.temperature = document["current"]["temperature_2m"] | NAN;
    out.weatherCode = document["current"]["weather_code"] | -1;
    out.valid = !isnan(out.temperature);
    out.lastRequestFailed = !out.valid;
    Serial.println(out.valid ? "Weather: updated" : "Weather: invalid response");
    return out.valid;
}

void WeatherService::update(unsigned long now) {
    // Publish whatever the worker finished since the last pass.
    WeatherData fetched;
    if (resultQueue_ != nullptr && xQueueReceive(resultQueue_, &fetched, 0) == pdTRUE) {
        if (fetched.valid) {
            data_.temperature = fetched.temperature;
            data_.weatherCode = fetched.weatherCode;
            data_.valid = true;
            data_.updatedAt = now;
        }
        data_.lastRequestFailed = fetched.lastRequestFailed;
        online_ = fetched.valid;
    }

    data_.requesting = fetchInFlight_;

    if (WiFi.status() != WL_CONNECTED) {
        online_ = false;
        return;
    }
    if (fetchInFlight_ || task_ == nullptr || resultQueue_ == nullptr) return;

    const unsigned long interval = data_.valid ? Config::WEATHER_REFRESH_MS : Config::WEATHER_RETRY_MS;
    if (lastAttemptAt_ != 0 && now - lastAttemptAt_ < interval) return;

    lastAttemptAt_ = now;
    fetchInFlight_ = true;
    data_.requesting = true;
    xTaskNotifyGive(task_);
}

const WeatherData& WeatherService::data() const { return data_; }
bool WeatherService::isOnline() const { return online_; }

const char* WeatherService::statusName() const {
    if (data_.requesting) return "requesting";
    if (online_) return "ok";
    if (WiFi.status() != WL_CONNECTED) return "no_internet";
    return "offline";
}
