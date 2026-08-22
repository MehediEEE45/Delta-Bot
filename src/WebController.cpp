#include "WebController.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <sys/time.h>
#include "Config.h"

namespace {
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Delta</title><style>
:root{--ink:#172026;--paper:#f4efe6;--accent:#e46b3e;--line:#cbbfaf}*{box-sizing:border-box}body{font-family:Georgia,serif;background:var(--paper);color:var(--ink);max-width:680px;margin:0 auto;padding:24px}header{border-bottom:3px solid var(--ink);display:flex;justify-content:space-between;align-items:end;padding-bottom:14px}h1{font-size:34px;margin:0}small,.status{font-family:monospace}.panel{border:1px solid var(--line);padding:16px;margin-top:18px;background:#fffaf2}h2{font-size:15px;text-transform:uppercase;letter-spacing:1px;margin:0 0 12px}button{background:var(--ink);color:white;border:0;padding:11px 14px;margin:4px;border-radius:3px;font-size:14px;cursor:pointer}button:hover,.active{background:var(--accent)}button:disabled{opacity:.45}#status{font-family:monospace;line-height:1.7;white-space:pre-line}
 :root{--ink:#172026;--paper:#f4efe6;--accent:#e46b3e;--line:#cbbfaf;--blue:#087bea;--red:#f51616}*{box-sizing:border-box}body{font-family:Georgia,serif;background:var(--paper);color:var(--ink);max-width:680px;margin:0 auto;padding:24px}header{border-bottom:3px solid var(--ink);display:flex;justify-content:space-between;align-items:end;padding-bottom:14px}h1{font-size:34px;margin:0}small,.status{font-family:monospace}.panel{border:1px solid var(--line);padding:16px;margin-top:18px;background:#fffaf2}h2{font-size:15px;text-transform:uppercase;letter-spacing:1px;margin:0 0 12px}button{background:var(--ink);color:white;border:0;padding:11px 14px;margin:4px;border-radius:3px;font-size:14px;cursor:pointer}button:hover,.active{background:var(--accent)}button:disabled{opacity:.45}#status{font-family:monospace;line-height:1.7;white-space:pre-line}.drive{background:#fff;border:1px solid #d8d8d8;padding:14px}.driveHead{display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid #bbb;padding-bottom:8px}.driveHead strong{font-family:Arial,sans-serif;font-size:16px}.driveState{font-family:monospace;font-size:12px;color:#087b35}.driveGrid{display:grid;grid-template-columns:repeat(3,74px);grid-template-rows:repeat(3,64px);gap:8px;justify-content:center;margin:16px auto}.driveBtn{background:var(--blue);border-radius:50%;font-size:0;width:64px;height:64px;position:relative;box-shadow:0 3px 0 #0560b5;touch-action:none}.driveBtn:active{transform:translateY(2px);box-shadow:none}.driveBtn:after{content:"";position:absolute;left:23px;top:20px;border-left:20px solid var(--red);border-top:12px solid transparent;border-bottom:12px solid transparent}.driveBtn.up:after{transform:rotate(-90deg);left:22px;top:18px}.driveBtn.down:after{transform:rotate(90deg);left:22px;top:14px}.driveBtn.left:after{transform:rotate(180deg);left:18px}.driveBtn.stop{background:#777;box-shadow:0 3px 0 #555;font-size:12px;font-weight:bold}.driveBtn.stop:after{display:none}.speedRow{font-family:monospace;text-align:center;font-size:13px}.speedRow input{width:100%;accent-color:#0a9b1d;background:linear-gradient(90deg,red,green)}
</style></head><body><header><h1>Delta</h1><small id="clock">--:--:--</small></header><div class="panel"><h2>Live status</h2><div id="status">Connecting...</div><button onclick="refreshWeather()">Refresh weather</button></div>
<div class="panel"><h2>Tabs</h2><button data-command="music" onclick="send('music')">Face / Music</button><button data-command="time" onclick="send('time')">Time + Date</button><button data-command="weather" onclick="send('weather')">Weather</button></div>
<div class="panel"><h2>Startup mode</h2><button onclick="setStartup('music')">Start with Music</button><button onclick="setStartup('time')">Start with Time</button><button onclick="setStartup('weather')">Start with Weather</button></div>
<div class="panel"><h2>Set clock</h2><input id="manualTime" type="datetime-local"><button onclick="setTime()">Set time and date</button></div>
<div class="panel"><h2>Wi-Fi settings</h2><input id="wifiSsid" placeholder="Wi-Fi name"><input id="wifiPassword" type="password" placeholder="Wi-Fi password"><button onclick="saveWiFi()">Save Wi-Fi and reboot</button></div>
<div class="panel"><h2>Drive</h2><label>Speed <input id="motorSpeed" type="range" min="0" max="255" value="180"></label><br><button onpointerdown="motor('forward')" onpointerup="releaseMotor()" onpointerleave="releaseMotor()">Forward</button><button onpointerdown="motor('left')" onpointerup="releaseMotor()" onpointerleave="releaseMotor()">Left</button><button onclick="releaseMotor()">Stop</button><button onpointerdown="motor('right')" onpointerup="releaseMotor()" onpointerleave="releaseMotor()">Right</button><button onpointerdown="motor('backward')" onpointerup="releaseMotor()" onpointerleave="releaseMotor()">Backward</button></div>
 <div class="panel drive"><div class="driveHead"><strong>Arduino RC Control Car</strong><span class="driveState" id="motorState">STOPPED</span></div><div class="speedRow">SPEED <span id="speedValue">70</span>%<input id="motorSpeed" type="range" min="0" max="255" value="180" oninput="document.querySelector('#speedValue').textContent=Math.round(this.value/255*100)"></div><div class="driveGrid"><span></span><button class="driveBtn up" aria-label="Forward" onpointerdown="motor('forward')" onpointerup="releaseMotor()" onpointerleave="releaseMotor()">Forward</button><span></span><button class="driveBtn left" aria-label="Left" onpointerdown="motor('left')" onpointerup="releaseMotor()" onpointerleave="releaseMotor()">Left</button><button class="driveBtn stop" onclick="releaseMotor()">PARK</button><button class="driveBtn right" aria-label="Right" onpointerdown="motor('right')" onpointerup="releaseMotor()" onpointerleave="releaseMotor()">Right</button><span></span><button class="driveBtn down" aria-label="Backward" onpointerdown="motor('backward')" onpointerup="releaseMotor()" onpointerleave="releaseMotor()">Backward</button><span></span></div></div>
<div class="panel"><h2>Emotions</h2><button data-command="happy" onclick="send('happy')">Happy</button><button data-command="love" onclick="send('love')">Love</button><button data-command="excited" onclick="send('excited')">Excited</button><button data-command="cool" onclick="send('cool')">Cool</button><button data-command="sad" onclick="send('sad')">Sad</button><button data-command="angry" onclick="send('angry')">Angry</button><button data-command="surprised" onclick="send('surprised')">Surprised</button><button data-command="sleep" onclick="send('sleep')">Sleep</button></div>
<script>let busy=false,motorTimer=0;async function send(command){if(busy)return;busy=true;document.querySelectorAll('[data-command]').forEach(b=>b.classList.toggle('active',b.dataset.command===command));try{await fetch('/api/command?value='+command,{method:'POST',signal:AbortSignal.timeout(1200)});refresh()}catch(error){}finally{busy=false}}async function motor(command){clearInterval(motorTimer);document.querySelector('#motorState').textContent=command.toUpperCase();let speed=document.querySelector('#motorSpeed').value;const sendMotor=()=>fetch('/api/motor?command='+command+'&speed='+speed,{method:'POST',signal:AbortSignal.timeout(700)}).catch(()=>{});await sendMotor();if(command!=='stop')motorTimer=setInterval(sendMotor,250)}function releaseMotor(){clearInterval(motorTimer);motorTimer=0;motor('stop')}async function setStartup(mode){await fetch('/api/settings?defaultMode='+mode,{method:'POST'});await refresh()}async function setTime(){let value=document.querySelector('#manualTime').value;if(value){let parts=value.split(/[-T:]/);let query='year='+parts[0]+'&month='+parts[1]+'&day='+parts[2]+'&hour='+parts[3]+'&minute='+parts[4];await fetch('/api/time?'+query,{method:'POST'});await refresh()}}async function saveWiFi(){let ssid=encodeURIComponent(document.querySelector('#wifiSsid').value);let password=encodeURIComponent(document.querySelector('#wifiPassword').value);if(ssid)await fetch('/api/wifi?ssid='+ssid+'&password='+password,{method:'POST'});document.querySelector('#status').textContent='Saved. Reconnecting...'}async function refreshWeather(){await fetch('/api/weather/refresh',{method:'POST'});await refresh()}async function refresh(){try{let response=await fetch('/api/status');let s=await response.json();document.querySelector('#status').textContent='MODE   '+s.mode+'\nFACE   '+s.emotion+'\nSTART  '+s.defaultMode+'\nMOTOR  '+s.motor+'\nNET    '+s.network+'\nWEATHER '+s.weather+' '+s.condition+' '+(s.temperature===null?'--':s.temperature+' C')+'\nUPDATED '+s.weatherUpdatedAt+' ms\nIP     '+s.ip;document.querySelector('#clock').textContent=s.time;document.querySelector('#motorState').textContent=s.motor.toUpperCase();document.querySelectorAll('[data-command]').forEach(b=>b.classList.toggle('active',b.dataset.command===s.mode||b.dataset.command===s.emotion))}catch(error){document.querySelector('#status').textContent='Delta unavailable'}}refresh();setInterval(refresh,2000)</script></body></html>
)rawliteral";
}

WebController::WebController(AppController& app, MotorController& motors) : app_(app), motors_(motors) {}

void WebController::begin() {
    registerRoutes();
    server_.begin();
}

void WebController::update() {
    server_.handleClient();
    if (restartRequested_) {
        delay(250);
        ESP.restart();
    }
}

void WebController::registerRoutes() {
    server_.on("/", HTTP_GET, [this]() { server_.send(200, "text/html", INDEX_HTML); });
    server_.onNotFound([this]() { server_.send(404, "text/plain", "Delta web server is running. Open http://192.168.4.1/"); });
    server_.on("/api/status", HTTP_GET, [this]() { sendStatus(); });
    server_.on("/api/weather/refresh", HTTP_POST, [this]() { refreshWeather(); });
    server_.on("/api/settings", HTTP_POST, [this]() { updateSettings(); });
    server_.on("/api/time", HTTP_POST, [this]() { updateTime(); });
    server_.on("/api/wifi", HTTP_POST, [this]() { updateWiFi(); });
    server_.on("/api/motor", HTTP_POST, [this]() { motorCommand(); });
    server_.on("/api/command", HTTP_POST, [this]() {
        const bool accepted = handleCommand(server_.arg("value"));
        server_.send(accepted ? 200 : 400, "application/json", accepted ? "{\"ok\":true}" : "{\"ok\":false}");
    });
}

void WebController::sendStatus() {
    JsonDocument document;
    document["mode"] = app_.modeName();
    document["emotion"] = app_.emotionName();
    document["motor"] = motors_.commandName();
    document["defaultMode"] = app_.defaultModeName();
    document["battery"] = app_.batteryPercent();
    document["network"] = WiFi.status() == WL_CONNECTED ? "online" : (WiFi.getMode() == WIFI_AP ? "offline_ap" : "offline");
    document["ip"] = WiFi.getMode() == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    time_t currentTime = time(nullptr);
    document["time"] = currentTime > 100000 ? String(ctime(&currentTime)).substring(0, 24) : "SYNCING";
    const WeatherData& weather = app_.weatherData();
    document["weather"] = weather.requesting ? "requesting" : (weather.lastRequestFailed && weather.valid ? "stale" : (weather.valid ? "ok" : (WiFi.status() == WL_CONNECTED ? "offline" : "no_internet")));
    document["condition"] = weather.valid ? weatherCodeText(weather.weatherCode) : "unknown";
    document["weatherUpdatedAt"] = weather.updatedAt;
    if (weather.valid) document["temperature"] = weather.temperature;
    else document["temperature"] = nullptr;
    String response;
    serializeJson(document, response);
    server_.send(200, "application/json", response);
}

void WebController::refreshWeather() {
    app_.requestWeatherRefresh();
    server_.send(202, "application/json", "{\"ok\":true,\"status\":\"queued\"}");
}

bool WebController::handleCommand(const String& command) {
    const unsigned long now = millis();
    if (command == "music") app_.setMode(AppMode::Music);
    else if (command == "time" || command == "clock") app_.setMode(AppMode::TimeDate);
    else if (command == "weather") app_.setMode(AppMode::Weather);
    else if (command == "happy") app_.setEmotion(Emotion::Happy, 0, now);
    else if (command == "love") app_.setEmotion(Emotion::Love, 0, now);
    else if (command == "excited") app_.setEmotion(Emotion::Excited, 0, now);
    else if (command == "cool") app_.setEmotion(Emotion::Cool, 0, now);
    else if (command == "sad") app_.setEmotion(Emotion::Sad, 0, now);
    else if (command == "angry") app_.setEmotion(Emotion::Angry, 0, now);
    else if (command == "surprised") app_.setEmotion(Emotion::Surprised, 5000, now);
    else if (command == "sleep") app_.setEmotion(Emotion::Sleep, 0, now);
    else return false;
    return true;
}

void WebController::updateSettings() {
    const String defaultMode = server_.arg("defaultMode");
    if (defaultMode == "music") app_.setDefaultMode(AppMode::Music);
    else if (defaultMode == "time") app_.setDefaultMode(AppMode::TimeDate);
    else if (defaultMode == "weather") app_.setDefaultMode(AppMode::Weather);
    else {
        server_.send(400, "application/json", "{\"ok\":false}");
        return;
    }
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::updateTime() {
    struct tm localTime = {};
    localTime.tm_year = server_.arg("year").toInt() - 1900;
    localTime.tm_mon = server_.arg("month").toInt() - 1;
    localTime.tm_mday = server_.arg("day").toInt();
    localTime.tm_hour = server_.arg("hour").toInt();
    localTime.tm_min = server_.arg("minute").toInt();
    localTime.tm_sec = 0;
    if (localTime.tm_year < 100 || localTime.tm_mon < 0 || localTime.tm_mon > 11 || localTime.tm_mday < 1 || localTime.tm_mday > 31 || localTime.tm_hour < 0 || localTime.tm_hour > 23 || localTime.tm_min < 0 || localTime.tm_min > 59) {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_local_time\"}");
        return;
    }
    setenv("TZ", Config::TIMEZONE, 1);
    tzset();
    const time_t epoch = mktime(&localTime);
    timeval timeValue = {epoch, 0};
    settimeofday(&timeValue, nullptr);
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::updateWiFi() {
    const String ssid = server_.arg("ssid");
    const String password = server_.arg("password");
    if (ssid.length() == 0 || ssid.length() > 64 || password.length() > 64) {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_wifi_settings\"}");
        return;
    }
    app_.setWiFiCredentials(ssid, password);
    restartRequested_ = true;
    server_.send(200, "application/json", "{\"ok\":true,\"restarting\":true}");
}

void WebController::motorCommand() {
    const String command = server_.arg("command");
    const int requestedSpeed = constrain(server_.arg("speed").toInt(), 0, 255);
    MotorCommand motorCommand = MotorCommand::Stop;
    if (command == "forward") motorCommand = MotorCommand::Forward;
    else if (command == "backward") motorCommand = MotorCommand::Backward;
    else if (command == "left") motorCommand = MotorCommand::Left;
    else if (command == "right") motorCommand = MotorCommand::Right;
    else if (command != "stop") {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_motor_command\"}");
        return;
    }
    motors_.drive(motorCommand, static_cast<uint8_t>(requestedSpeed));
    const unsigned long now = millis();
    if (motorCommand == MotorCommand::Forward) app_.setEmotion(Emotion::Excited, 0, now);
    else if (motorCommand == MotorCommand::Backward) app_.setEmotion(Emotion::Sad, 0, now);
    else if (motorCommand == MotorCommand::Left || motorCommand == MotorCommand::Right) app_.setEmotion(Emotion::Cool, 0, now);
    else app_.setEmotion(Emotion::Happy, 0, now);
    server_.send(200, "application/json", "{\"ok\":true}");
}
