#include <Arduino.h>
#include <WiFi.h>
#include "AppController.h"
#include "BleConsole.h"
#include "CommandProcessor.h"
#include "Config.h"
#include "FaceRenderer.h"
#include "SerialConsole.h"
#include "TouchSensor.h"
#include "WebController.h"
#include "Weather.h"
#include "WiFiDiagnostics.h"
#include "MotorController.h"

FaceRenderer renderer;
WeatherService weather;
TouchSensor touch(Config::TOUCH_PIN);
AppController app(renderer, weather);
MotorController motors(Config::MOTOR_LEFT_PWM, Config::MOTOR_LEFT_IN1, Config::MOTOR_LEFT_IN2, Config::MOTOR_RIGHT_PWM, Config::MOTOR_RIGHT_IN1, Config::MOTOR_RIGHT_IN2, Config::MOTOR_STBY);
CommandProcessor commands(app, motors);
WebController web(app, motors, commands);
SerialConsole serialConsole(commands);
BleConsole bleConsole(commands);

// What the OLED should tell the user once the radio has settled. Filled in by
// the bring-up path so the panel can repeat what used to be serial-only.
struct NetworkInfo {
  bool valid = false;
  bool apMode = false;
  String ssid;
  String password; // AP mode only; empty means the AP came up open
  String ip;
};

bool startFallbackAccessPoint(NetworkInfo& info) {
  WiFi.disconnect(true, true);
  delay(100);
  WiFi.mode(WIFI_AP);
  IPAddress apIp(192, 168, 4, 1);
  IPAddress apGateway(192, 168, 4, 1);
  IPAddress apSubnet(255, 255, 255, 0);
  WiFi.softAPConfig(apIp, apGateway, apSubnet);

  info.apMode = true;
  info.ssid = Config::FALLBACK_AP_SSID;

  if (WiFi.softAP(Config::FALLBACK_AP_SSID, Config::FALLBACK_AP_PASSWORD)) {
    Serial.print("Fallback website: http://");
    Serial.println(WiFi.softAPIP());
    Serial.print("Connect to Wi-Fi network: ");
    Serial.println(Config::FALLBACK_AP_SSID);
    Serial.println("Fallback password: (see include/Secrets.h)");
    info.valid = true;
    info.password = Config::FALLBACK_AP_PASSWORD;
    info.ip = WiFi.softAPIP().toString();
    web.begin();
    return true;
  }

  Serial.println("Secured fallback AP failed; retrying as open network.");
  if (WiFi.softAP(Config::FALLBACK_AP_SSID)) {
    Serial.print("Open fallback website: http://");
    Serial.println(WiFi.softAPIP());
    Serial.print("Connect to open Wi-Fi network: ");
    Serial.println(Config::FALLBACK_AP_SSID);
    info.valid = true;
    info.password = "";
    info.ip = WiFi.softAPIP().toString();
    web.begin();
    return true;
  }

  Serial.println("Fallback network failed to start.");
  return false;
}

// runWiFiDiagnostics() takes a plain function pointer, so the display it needs
// to keep alive is reached through the file-scope renderer rather than a
// capture.
void paintBootProgress(const char* headline, const char* detail) {
  renderer.showBootStatus(headline, detail, millis());
}

void connectWiFi(NetworkInfo& info) {
  String ssid = app.wifiSsid();
  String password = app.wifiPassword();
  ssid.trim();
  password.trim();

  if (ssid.isEmpty() || ssid == "YOUR_WIFI_SSID") {
    Serial.println("Wi-Fi not configured; starting fallback network.");
    renderer.showBootStatus("no wi-fi set", "starting setup ap", millis());
    startFallbackAccessPoint(info);
    return;
  }

  WiFi.setAutoReconnect(true);
  WiFi.disconnect(true, true);
  delay(150);

  const WiFiDiagnostics diag = runWiFiDiagnostics(ssid, password, 20000, paintBootProgress);
  commands.setLastDiagnostics(diag);
  Serial.println();
  diag.report(Serial);

  if (diag.fault == WiFiFault::None) {
    configTzTime(app.timezone(), "pool.ntp.org", "time.nist.gov");
    Serial.print("Delta web server: http://");
    Serial.println(WiFi.localIP());
    info.valid = true;
    info.apMode = false;
    info.ssid = ssid;
    info.ip = WiFi.localIP().toString();
    web.begin();
    return;
  }

  // Hold the reason on the panel before the setup card replaces it, so a
  // failure is readable without a laptop attached.
  if (renderer.ready()) {
    renderer.showWiFiDiagScreen(diag);
    delay(Config::WIFI_DIAG_CARD_MS);
  }
  Serial.println("Wi-Fi connection failed; starting Delta fallback network.");
  startFallbackAccessPoint(info);
}

// Plays the boot name card through to its settled frame. setup() has nothing
// else to do while it runs, so this blocks rather than threading splash state
// through loop().
void playSplash() {
  const unsigned long startedAt = millis();
  for (;;) {
    const unsigned long elapsed = millis() - startedAt;
    renderer.showSplash(elapsed);
    if (elapsed >= Config::SPLASH_HOLD_MS) return;
    delay(Config::SPLASH_FRAME_MS);
  }
}

// Leaves the SSID, password and address on screen long enough to be typed into
// a phone. The web server is already listening by this point, so keep pumping
// it instead of blocking on a bare delay.
void holdNetworkCard(const NetworkInfo& info) {
  if (!info.valid || !renderer.ready()) return;
  renderer.showNetworkCard(info.apMode, info.ssid, info.password, info.ip);
  const unsigned long startedAt = millis();
  while (millis() - startedAt < Config::NETWORK_CARD_MS) {
    web.update();
    delay(10);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Delta booting");
  if (!renderer.begin()) {
    Serial.println("Continuing headless; the web UI still works.");
  } else {
    // Name card first, so a bare panel proves itself before Wi-Fi is tried.
    playSplash();
  }
  touch.begin();
  weather.begin();
  motors.begin();
  app.begin();
  NetworkInfo network;
  connectWiFi(network);
  holdNetworkCard(network);
  // BLE comes up after Wi-Fi so the radio is not being shared during the
  // association attempt the diagnostics depend on.
  bleConsole.begin();
  serialConsole.begin();
}

void loop() {
  web.update();
  const unsigned long now = millis();
  serialConsole.update();
  bleConsole.update();
  const TouchEvent touchEvent = touch.update(now);
  app.handleTouch(touchEvent, now);
  // Desk Guard warning pulse: a caught intruder gets a wheel wiggle. main owns
  // both objects, so the alarm stays decoupled from the motor driver.
  if (app.taskManager().consumeGuardAlarmPulse()) {
    motors.startShake(now);
  }
  web.update();
  motors.update(now);
  app.update(now);
}