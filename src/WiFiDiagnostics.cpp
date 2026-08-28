#include "WiFiDiagnostics.h"
#include "Config.h"

namespace {
// A working ESP32 radio reports a burned-in MAC. All-zero means the driver
// never got one; all-FF means the read failed outright. Either way the PHY is
// not usable, which is the one failure no amount of retrying will fix.
bool macLooksReal(const String& mac) {
    if (mac.length() != 17) return false;
    bool sawNonZero = false;
    bool sawNonFf = false;
    for (unsigned int i = 0; i < mac.length(); i += 3) {
        const String byteText = mac.substring(i, i + 2);
        if (byteText != "00") sawNonZero = true;
        if (byteText != "FF" && byteText != "ff") sawNonFf = true;
    }
    return sawNonZero && sawNonFf;
}
}

const char* wifiStatusName(wl_status_t status) {
    switch (status) {
        case WL_IDLE_STATUS: return "IDLE";
        case WL_SCAN_COMPLETED: return "SCAN_COMPLETED";
        case WL_CONNECTED: return "CONNECTED";
        case WL_NO_SSID_AVAIL: return "NO_SSID_AVAILABLE";
        case WL_CONNECT_FAILED: return "CONNECT_FAILED";
        case WL_CONNECTION_LOST: return "CONNECTION_LOST";
        case WL_DISCONNECTED: return "DISCONNECTED";
        case WL_NO_SHIELD: return "NO_SHIELD";
        default: return "OTHER";
    }
}

const char* wifiFaultName(WiFiFault fault) {
    switch (fault) {
        case WiFiFault::None: return "ok";
        case WiFiFault::RadioDead: return "radio-dead";
        case WiFiFault::ScanFoundNone: return "scan-empty";
        case WiFiFault::SsidNotFound: return "ssid-not-found";
        case WiFiFault::AuthFailed: return "auth-failed";
        case WiFiFault::Timeout: return "timeout";
        case WiFiFault::NotConfigured: return "not-configured";
    }
    return "unknown";
}

const char* wifiFaultSummary(WiFiFault fault) {
    switch (fault) {
        case WiFiFault::None: return "connected";
        case WiFiFault::RadioDead: return "RADIO FAULT - check hw";
        case WiFiFault::ScanFoundNone: return "no networks at all";
        case WiFiFault::SsidNotFound: return "radio ok, ssid absent";
        case WiFiFault::AuthFailed: return "wrong password?";
        case WiFiFault::Timeout: return "ssid seen, no answer";
        case WiFiFault::NotConfigured: return "no ssid configured";
    }
    return "unknown";
}

void WiFiDiagnostics::report(Print& out) const {
    if (!valid) {
        out.println("wifi: no diagnostic captured yet");
        return;
    }
    out.print("wifi fault : ");
    out.print(wifiFaultName(fault));
    out.print(" (");
    out.print(wifiFaultSummary(fault));
    out.println(")");
    out.print("mac        : ");
    out.print(mac);
    out.println(macPlausible ? "  [plausible]" : "  [IMPLAUSIBLE - radio suspect]");
    out.print("scan found : ");
    out.print(scanCount);
    out.println(" network(s)");
    out.print("target ssid: ");
    if (targetFound) {
        out.print("found, rssi ");
        out.print(targetRssi);
        out.println(" dBm");
    } else {
        out.println("not seen in scan");
    }
    out.print("last status: ");
    out.println(wifiStatusName(lastStatus));
    out.print("captured   : ");
    out.print(capturedAtMs / 1000);
    out.println("s since boot");
}

WiFiDiagnostics runWiFiDiagnostics(const String& ssid, const String& password, unsigned long timeoutMs,
                                   void (*onProgress)(const char* headline, const char* detail)) {
    WiFiDiagnostics diag;
    diag.valid = true;

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    delay(50);

    const String macText = WiFi.macAddress();
    strncpy(diag.mac, macText.c_str(), sizeof(diag.mac) - 1);
    diag.macPlausible = macLooksReal(macText);
    if (!diag.macPlausible) {
        // Nothing downstream is meaningful if the PHY never came up, so stop
        // here rather than reporting a misleading "ssid not found".
        diag.fault = WiFiFault::RadioDead;
        diag.capturedAtMs = millis();
        return diag;
    }

    if (onProgress) onProgress("scanning", "for networks");
    diag.scanCount = WiFi.scanNetworks();
    if (diag.scanCount > 0 && !ssid.isEmpty()) {
        for (int i = 0; i < diag.scanCount; ++i) {
            if (WiFi.SSID(i) == ssid) {
                diag.targetFound = true;
                diag.targetRssi = WiFi.RSSI(i);
                break;
            }
        }
    }
    WiFi.scanDelete();

    if (diag.scanCount <= 0) {
        // A radio that reports a real MAC but cannot see a single network is
        // most often an antenna fault rather than a dead chip.
        diag.fault = WiFiFault::ScanFoundNone;
        diag.capturedAtMs = millis();
        return diag;
    }

    if (ssid.isEmpty()) {
        diag.fault = WiFiFault::NotConfigured;
        diag.capturedAtMs = millis();
        return diag;
    }

    if (onProgress) onProgress("connecting to", ssid.c_str());
    if (password.isEmpty()) {
        WiFi.begin(ssid.c_str());
    } else {
        WiFi.begin(ssid.c_str(), password.c_str());
    }

    const unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startedAt < timeoutMs) {
        if (onProgress) onProgress("connecting to", ssid.c_str());
        delay(Config::BOOT_STATUS_FRAME_MS);
    }

    diag.lastStatus = WiFi.status();
    diag.capturedAtMs = millis();

    if (diag.lastStatus == WL_CONNECTED) diag.fault = WiFiFault::None;
    else if (!diag.targetFound) diag.fault = WiFiFault::SsidNotFound;
    else if (diag.lastStatus == WL_CONNECT_FAILED) diag.fault = WiFiFault::AuthFailed;
    else diag.fault = WiFiFault::Timeout;

    return diag;
}
