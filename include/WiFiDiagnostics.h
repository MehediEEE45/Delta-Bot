#pragma once

#include <Arduino.h>
#include <WiFi.h>

// Why a connection attempt ended the way it did. The point of this enum is to
// separate "the radio itself is not working" from the ordinary reasons a join
// fails, which otherwise all look the same from the outside.
enum class WiFiFault {
    None,           // associated successfully
    RadioDead,      // MAC is all-zero/all-FF, or the driver refused to start
    ScanFoundNone,  // radio works, but the scan returned zero networks at all
    SsidNotFound,   // other networks visible, target SSID absent
    AuthFailed,     // target seen, association rejected
    Timeout,        // target seen, association never completed
    NotConfigured   // no SSID set, so nothing was attempted
};

const char* wifiFaultName(WiFiFault fault);
// One-line human summary, e.g. "radio ok, SSID not in range".
const char* wifiFaultSummary(WiFiFault fault);
const char* wifiStatusName(wl_status_t status);

struct WiFiDiagnostics {
    bool valid = false;
    WiFiFault fault = WiFiFault::None;
    char mac[18] = {};
    bool macPlausible = false;
    int scanCount = -1;
    bool targetFound = false;
    int targetRssi = 0;
    wl_status_t lastStatus = WL_IDLE_STATUS;
    unsigned long capturedAtMs = 0;

    // Writes the full multi-line report to any Print sink -- Serial, a BLE
    // notify adapter or an HTTP response body all satisfy the same interface.
    void report(Print& out) const;
};

// Runs a full radio check: MAC sanity, scan, and (when `ssid` is non-empty) an
// association attempt. Blocks for the duration of the scan plus `timeoutMs`.
// `onProgress` may be null; it is called with a short status string so a caller
// can keep a display alive while this runs.
WiFiDiagnostics runWiFiDiagnostics(const String& ssid, const String& password, unsigned long timeoutMs,
                                   void (*onProgress)(const char* headline, const char* detail));
