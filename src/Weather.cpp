#include "Weather.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "Config.h"

void WeatherService::begin() {
    lastAttemptAt_ = 0;
}

void WeatherService::requestRefresh() {
    lastAttemptAt_ = 0;
}

void WeatherService::update(unsigned long now) {
    if (WiFi.status() != WL_CONNECTED) {
        online_ = false;
        data_.requesting = false;
        return;
    }
    if (lastAttemptAt_ != 0 && now - lastAttemptAt_ < (data_.valid ? Config::WEATHER_REFRESH_MS : Config::WEATHER_RETRY_MS)) return;
    lastAttemptAt_ = now;
    data_.requesting = true;

    String url = "https://api.open-meteo.com/v1/forecast?latitude=";
    url += Config::WEATHER_LATITUDE;
    url += "&longitude=";
    url += Config::WEATHER_LONGITUDE;
    url += "&current=temperature_2m,weather_code&timezone=auto";

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setTimeout(1000);
    if (!http.begin(client, url)) {
        Serial.println("Weather: HTTPS connection setup failed");
        online_ = false;
        data_.requesting = false;
        data_.lastRequestFailed = true;
        return;
    }
    const int result = http.GET();
    if (result == HTTP_CODE_OK) {
        JsonDocument document;
        DeserializationError error = deserializeJson(document, http.getString());
        if (!error) {
            data_.temperature = document["current"]["temperature_2m"] | NAN;
            data_.weatherCode = document["current"]["weather_code"] | -1;
            data_.valid = !isnan(data_.temperature);
            data_.updatedAt = now;
            online_ = data_.valid;
            data_.lastRequestFailed = !data_.valid;
            Serial.println(online_ ? "Weather: updated" : "Weather: invalid response");
        } else {
            Serial.print("Weather: JSON error: ");
            Serial.println(error.c_str());
            online_ = false;
            data_.lastRequestFailed = true;
        }
    } else {
        Serial.print("Weather: HTTP error ");
        Serial.println(result);
        online_ = false;
        data_.lastRequestFailed = true;
    }
    data_.requesting = false;
    http.end();
}

const WeatherData& WeatherService::data() const { return data_; }
bool WeatherService::isOnline() const { return online_; }

const char* WeatherService::statusName() const {
    if (data_.requesting) return "requesting";
    if (online_) return "ok";
    if (WiFi.status() != WL_CONNECTED) return "no_internet";
    return "offline";
}