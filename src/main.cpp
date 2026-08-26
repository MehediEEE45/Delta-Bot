#include <Arduino.h>
#include <WiFi.h>
#include "AppController.h"
#include "Config.h"
#include "FaceRenderer.h"
#include "TouchSensor.h"
#include "WebController.h"
#include "Weather.h"
#include "MotorController.h"

FaceRenderer renderer;
WeatherService weather;
TouchSensor touch(Config::TOUCH_PIN);
AppController app(renderer, weather);
MotorController motors(Config::MOTOR_LEFT_PWM, Config::MOTOR_LEFT_IN1, Config::MOTOR_LEFT_IN2, Config::MOTOR_RIGHT_PWM, Config::MOTOR_RIGHT_IN1, Config::MOTOR_RIGHT_IN2, Config::MOTOR_STBY);
WebController web(app, motors);

bool startFallbackAccessPoint() {
  WiFi.disconnect(true, true);
  delay(100);
  WiFi.mode(WIFI_AP);
  IPAddress apIp(192, 168, 4, 1);
  IPAddress apGateway(192, 168, 4, 1);
  IPAddress apSubnet(255, 255, 255, 0);
  WiFi.softAPConfig(apIp, apGateway, apSubnet);

  if (WiFi.softAP(Config::FALLBACK_AP_SSID, Config::FALLBACK_AP_PASSWORD)) {
    Serial.print("Fallback website: http://");
    Serial.println(WiFi.softAPIP());
    Serial.print("Connect to Wi-Fi network: ");
    Serial.println(Config::FALLBACK_AP_SSID);
    Serial.println("Fallback password: (see include/Secrets.h)");
    web.begin();
    return true;
  }

  Serial.println("Secured fallback AP failed; retrying as open network.");
  if (WiFi.softAP(Config::FALLBACK_AP_SSID)) {
    Serial.print("Open fallback website: http://");
    Serial.println(WiFi.softAPIP());
    Serial.print("Connect to open Wi-Fi network: ");
    Serial.println(Config::FALLBACK_AP_SSID);
    web.begin();
    return true;
  }

  Serial.println("Fallback network failed to start.");
  return false;
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

void connectWiFi() {
  String ssid = app.wifiSsid();
  String password = app.wifiPassword();
  ssid.trim();
  password.trim();

  if (ssid.isEmpty() || ssid == "YOUR_WIFI_SSID") {
    Serial.println("Wi-Fi not configured; starting fallback network.");
    startFallbackAccessPoint();
    return;
  }

  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(150);
  Serial.println("Scanning Wi-Fi networks...");
  const int networkCount = WiFi.scanNetworks();
  bool ssidSeen = false;
  for (int index = 0; index < networkCount; ++index) {
    Serial.print("  Found: ");
    const String foundSsid = WiFi.SSID(index);
    Serial.println(foundSsid);
    if (foundSsid == ssid) {
      ssidSeen = true;
    }
  }
  WiFi.scanDelete();
  if (!ssidSeen) {
    Serial.println("Configured SSID was not found in scan (it may be hidden or out of range).");
  }

  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);
  if (password.isEmpty()) {
    WiFi.begin(ssid.c_str());
  } else {
    WiFi.begin(ssid.c_str(), password.c_str());
  }
  Serial.print("Connecting to Wi-Fi");
  const unsigned long startedAt = millis();
  wl_status_t lastStatus = WL_IDLE_STATUS;
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 20000) {
    delay(250);
    Serial.print('.');
    const wl_status_t currentStatus = WiFi.status();
    if (currentStatus != lastStatus) {
      Serial.print(" status=");
      Serial.print(static_cast<int>(currentStatus));
      Serial.print(" (");
      Serial.print(wifiStatusName(currentStatus));
      Serial.print(')');
      lastStatus = currentStatus;
    }
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    configTzTime(Config::TIMEZONE, "pool.ntp.org", "time.nist.gov");
    Serial.print("Delta web server: http://");
    Serial.println(WiFi.localIP());
    web.begin();
  } else {
    Serial.println("Wi-Fi connection failed; starting Delta fallback network.");
    startFallbackAccessPoint();
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Delta booting");
  if (!renderer.begin()) {
    Serial.println("Continuing headless; the web UI still works.");
  }
  touch.begin();
  weather.begin();
  motors.begin();
  app.begin();
  connectWiFi();
}

void loop() {
  const unsigned long now = millis();
  static unsigned long lastHeartbeatAt = 0;
  if (now - lastHeartbeatAt >= 5000) {
    lastHeartbeatAt = now;
    Serial.print("Heartbeat, Wi-Fi status: ");
    const wl_status_t status = WiFi.status();
    Serial.print(static_cast<int>(status));
    Serial.print(" (");
    Serial.print(wifiStatusName(status));
    Serial.print(") mode=");
    Serial.println(WiFi.getMode() == WIFI_AP ? "AP" : (WiFi.getMode() == WIFI_STA ? "STA" : "AP_STA"));
  }
  const TouchEvent touchEvent = touch.update(now);
  if (touchEvent != TouchEvent::None) {
    Serial.print("Touch event: ");
    Serial.println(touchEventName(touchEvent));
  }
  app.handleTouch(touchEvent, now);
  web.update();
  motors.update(now);
  app.update(now);
}