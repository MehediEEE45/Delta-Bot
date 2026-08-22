#include "WebController.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <sys/time.h>
#include "Config.h"

namespace {
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Delta-Bot Control Center</title><style>
:root{--ink:#172026;--paper:#f4efe6;--accent:#e46b3e;--line:#cbbfaf;--blue:#087bea;--green:#0a9b1d}*{box-sizing:border-box}body{font-family:system-ui,sans-serif;background:var(--paper);color:var(--ink);max-width:720px;margin:0 auto;padding:16px}header{border-bottom:3px solid var(--ink);display:flex;justify-content:space-between;align-items:center;padding-bottom:12px}h1{font-size:26px;margin:0}.panel{border:1px solid var(--line);padding:14px;margin-top:14px;background:#fffaf2;border-radius:6px}h2{font-size:14px;text-transform:uppercase;letter-spacing:1px;margin:0 0 10px;color:var(--accent)}button{background:var(--ink);color:#fff;border:0;padding:8px 12px;margin:3px;border-radius:4px;font-size:13px;cursor:pointer}button:hover,.active{background:var(--accent)}input{padding:8px;margin:4px 0;border:1px solid var(--line);border-radius:4px;width:100%}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:6px}.driveGrid{display:grid;grid-template-columns:repeat(3,60px);gap:6px;justify-content:center;margin:10px auto}.driveBtn{background:var(--blue);height:50px;font-size:12px;border-radius:6px}.canvasGrid{display:grid;grid-template-columns:repeat(16,1fr);gap:1px;background:#ccc;padding:2px;max-width:320px;margin:10px auto}.cell{background:#fff;aspect-ratio:1;cursor:pointer}.cell.on{background:#000}
</style></head><body>
<header><h1>🤖 Delta-Bot</h1><small id="clock">--:--:--</small></header>

<div class="panel"><h2>Live Status</h2><div id="status" style="font-family:monospace;white-space:pre-line">Connecting...</div><button onclick="refresh()">Refresh</button></div>

<div class="panel"><h2>🏎️ RC Car Drive</h2>
<div style="text-align:center">SPEED <input id="motorSpeed" type="range" min="0" max="255" value="180"></div>
<div class="driveGrid">
<span></span><button class="driveBtn" onpointerdown="motor('forward')" onpointerup="releaseMotor()">▲ FW</button><span></span>
<button class="driveBtn" onpointerdown="motor('left')" onpointerup="releaseMotor()">◄ LT</button>
<button class="driveBtn" style="background:#555" onclick="releaseMotor()">STOP</button>
<button class="driveBtn" onpointerdown="motor('right')" onpointerup="releaseMotor()">RT ►</button>
<span></span><button class="driveBtn" onpointerdown="motor('backward')" onpointerup="releaseMotor()">▼ BW</button><span></span>
</div></div>

<div class="panel"><h2>🎙️ Web Voice Command</h2>
<button onclick="startVoice()" style="background:var(--green)">🎤 Speak Command</button><span id="voiceText"></span></div>

<div class="panel"><h2>🎭 Emotions & Modes</h2>
<div class="grid">
<button onclick="send('happy')">😊 Happy</button><button onclick="send('love')">💖 Love</button><button onclick="send('angry')">😡 Angry</button>
<button onclick="send('cool')">😎 Cool</button><button onclick="send('surprised')">😲 Surprise</button><button onclick="send('sleep')">😴 Sleep</button>
<button onclick="send('time')">⏰ Time</button><button onclick="send('weather')">🌤️ Weather</button><button onclick="send('music')">🎵 Music</button>
<button onclick="send('tasks')">📋 Tasks</button><button onclick="send('notice')">📢 Notice</button><button onclick="send('pomodoro')">⏱️ Pomodoro</button>
<button onclick="send('quotes')">💡 Quotes</button><button onclick="send('pet')">🐶 Pet Mode</button><button onclick="send('decision')">🎲 8-Ball</button>
</div></div>

<div class="panel"><h2>🐶 Virtual Pet Station</h2>
<button onclick="petAction('feed')">🍕 Feed Pizza</button><button onclick="petAction('pet')">🖐️ Pet Head</button></div>

<div class="panel"><h2>🎲 Magic 8-Ball Decision Maker</h2>
<input id="askText" placeholder="Ask a YES/NO question..."><button onclick="askQuestion()">Ask Delta-Bot</button></div>

<div class="panel"><h2>📋 Task Manager</h2>
<input id="taskText" placeholder="New task..."><button onclick="addTask()">Add Task</button>
<div id="taskList"></div></div>

<div class="panel"><h2>📢 Broadcast Notice</h2>
<input id="noticeMsg" placeholder="Notice message..."><button onclick="sendNotice()">Send Notice</button></div>

<div class="panel"><h2>⏱️ Pomodoro Timer</h2>
<button onclick="pomoAction('start')">▶️ Start Work (25m)</button><button onclick="pomoAction('reset')">⏹️ Reset</button></div>

<div class="panel"><h2>🎨 Pixel Art Canvas</h2>
<div class="canvasGrid" id="canvasGrid"></div><button onclick="clearCanvas()">Clear Canvas</button></div>

<div class="panel"><h2>🕵️‍♂️ Desk Guard Security</h2>
<button onclick="toggleGuard(true)">🛡️ Arm Guard</button><button onclick="toggleGuard(false)">🔓 Disarm</button></div>

<div class="panel"><h2>⚙️ Wi-Fi Setup</h2>
<input id="wifiSsid" placeholder="SSID"><input id="wifiPassword" type="password" placeholder="Password"><button onclick="saveWiFi()">Save & Reboot</button></div>

<script>
let busy=false,motorTimer=0,pixels=new Array(256).fill(0);
const grid=document.getElementById('canvasGrid');
for(let i=0;i<256;i++){let c=document.createElement('div');c.className='cell';c.onclick=()=>{pixels[i]=pixels[i]?0:1;c.classList.toggle('on',pixels[i]);sendCanvas()};grid.appendChild(c)}
async function send(c){await fetch('/api/command?value='+c,{method:'POST'});refresh()}
async function motor(c){clearInterval(motorTimer);let s=document.getElementById('motorSpeed').value;fetch('/api/motor?command='+c+'&speed='+s,{method:'POST'});if(c!=='stop')motorTimer=setInterval(()=>fetch('/api/motor?command='+c+'&speed='+s,{method:'POST'}),250)}
function releaseMotor(){clearInterval(motorTimer);motor('stop')}
async function addTask(){let t=document.getElementById('taskText').value;if(t){await fetch('/api/tasks?text='+encodeURIComponent(t),{method:'POST'});document.getElementById('taskText').value='';refresh()}}
async function sendNotice(){let m=document.getElementById('noticeMsg').value;if(m){await fetch('/api/notice?text='+encodeURIComponent(m),{method:'POST'});refresh()}}
async function pomoAction(a){await fetch('/api/pomodoro?action='+a,{method:'POST'});refresh()}
async function petAction(a){await fetch('/api/pet?action='+a,{method:'POST'});refresh()}
async function askQuestion(){let q=document.getElementById('askText').value;if(q){await fetch('/api/decision?q='+encodeURIComponent(q),{method:'POST'});refresh()}}
async function toggleGuard(a){await fetch('/api/guard?armed='+a,{method:'POST'});refresh()}
async function sendCanvas(){await fetch('/api/canvas',{method:'POST',body:JSON.stringify(pixels)})}
async function clearCanvas(){pixels.fill(0);document.querySelectorAll('.cell').forEach(c=>c.classList.remove('on'));sendCanvas()}
async function saveWiFi(){let s=encodeURIComponent(document.getElementById('wifiSsid').value);let p=encodeURIComponent(document.getElementById('wifiPassword').value);if(s)await fetch('/api/wifi?ssid='+s+'&password='+p,{method:'POST'})}
function startVoice(){if(!('webkitSpeechRecognition' in window)){alert('Speech Recognition not supported in this browser.');return}let r=new webkitSpeechRecognition();r.onresult=e=>{let t=e.results[0][0].transcript.toLowerCase();document.getElementById('voiceText').textContent=' Heard: "'+t+'"';if(t.includes('forward'))motor('forward');else if(t.includes('back'))motor('backward');else if(t.includes('left'))motor('left');else if(t.includes('right'))motor('right');else if(t.includes('stop'))releaseMotor();else if(t.includes('happy'))send('happy');else if(t.includes('sleep'))send('sleep');else if(t.includes('weather'))send('weather')};r.start()}
async function refresh(){try{let r=await fetch('/api/status');let s=await r.json();document.getElementById('status').textContent='MODE: '+s.mode+' | FACE: '+s.emotion+' | BATT: '+s.battery+'%';document.getElementById('clock').textContent=s.time}catch(e){}}
setInterval(refresh,2000);refresh();
</script></body></html>
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

    server_.on("/api/tasks", HTTP_POST, [this]() { handleTasksApi(); });
    server_.on("/api/notice", HTTP_POST, [this]() { handleNoticeApi(); });
    server_.on("/api/reminder", HTTP_POST, [this]() { handleReminderApi(); });
    server_.on("/api/pomodoro", HTTP_POST, [this]() { handlePomodoroApi(); });
    server_.on("/api/pet", HTTP_POST, [this]() { handlePetApi(); });
    server_.on("/api/decision", HTTP_POST, [this]() { handleDecisionApi(); });
    server_.on("/api/canvas", HTTP_POST, [this]() { handleCanvasApi(); });
    server_.on("/api/guard", HTTP_POST, [this]() { handleGuardApi(); });
}

void WebController::sendStatus() {
    JsonDocument document;
    document["mode"] = app_.modeName();
    document["emotion"] = app_.emotionName();
    document["motor"] = motors_.commandName();
    document["defaultMode"] = app_.defaultModeName();
    document["battery"] = app_.batteryPercent();
    document["network"] = WiFi.status() == WL_CONNECTED ? "online" : "offline";
    document["ip"] = WiFi.getMode() == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    time_t currentTime = time(nullptr);
    document["time"] = currentTime > 100000 ? String(ctime(&currentTime)).substring(0, 24) : "SYNCING";

    String response;
    serializeJson(document, response);
    server_.send(200, "application/json", response);
}

void WebController::refreshWeather() {
    app_.requestWeatherRefresh();
    server_.send(202, "application/json", "{\"ok\":true}");
}

bool WebController::handleCommand(const String& command) {
    const unsigned long now = millis();
    if (command == "music") app_.setMode(AppMode::Music);
    else if (command == "time" || command == "clock") app_.setMode(AppMode::TimeDate);
    else if (command == "weather") app_.setMode(AppMode::Weather);
    else if (command == "tasks") app_.setMode(AppMode::Tasks);
    else if (command == "notice") app_.setMode(AppMode::Notice);
    else if (command == "reminder") app_.setMode(AppMode::Reminder);
    else if (command == "pomodoro") app_.setMode(AppMode::Pomodoro);
    else if (command == "canvas") app_.setMode(AppMode::Canvas);
    else if (command == "quotes") app_.setMode(AppMode::Quotes);
    else if (command == "pet") app_.setMode(AppMode::Pet);
    else if (command == "decision") app_.setMode(AppMode::Decision);
    else if (command == "happy") app_.setEmotion(Emotion::Happy, 0, now);
    else if (command == "love") app_.setEmotion(Emotion::Love, 0, now);
    else if (command == "angry") app_.setEmotion(Emotion::Angry, 0, now);
    else if (command == "cool") app_.setEmotion(Emotion::Cool, 0, now);
    else if (command == "surprised") app_.setEmotion(Emotion::Surprised, 5000, now);
    else if (command == "sleep") app_.setEmotion(Emotion::Sleep, 0, now);
    else return false;
    return true;
}

void WebController::handleTasksApi() {
    String text = server_.arg("text");
    if (text.length() > 0) {
        app_.taskManager().addTask(text);
        app_.setMode(AppMode::Tasks);
    }
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::handleNoticeApi() {
    String text = server_.arg("text");
    if (text.length() > 0) {
        app_.taskManager().setNotice(text);
        app_.setMode(AppMode::Notice);
    }
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::handleReminderApi() {
    String title = server_.arg("title");
    int minutes = server_.arg("minutes").toInt();
    if (title.length() > 0 && minutes > 0) {
        time_t target = time(nullptr) + minutes * 60;
        app_.taskManager().setReminder(title, target);
        app_.setMode(AppMode::Reminder);
    }
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::handlePomodoroApi() {
    String action = server_.arg("action");
    unsigned long now = millis();
    if (action == "start") {
        app_.taskManager().startPomodoro(now);
        app_.setMode(AppMode::Pomodoro);
    } else if (action == "reset") {
        app_.taskManager().resetPomodoro();
    }
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::handlePetApi() {
    String action = server_.arg("action");
    if (action == "feed") {
        app_.taskManager().feedPet();
        app_.setEmotion(Emotion::Happy, 3000, millis());
    } else if (action == "pet") {
        app_.taskManager().petPet();
        app_.setEmotion(Emotion::Love, 3000, millis());
    }
    app_.setMode(AppMode::Pet);
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::handleDecisionApi() {
    String q = server_.arg("q");
    if (q.length() > 0) {
        app_.taskManager().askDecision(q);
        app_.setMode(AppMode::Decision);
        motors_.drive(MotorCommand::Left, 150);
        delay(150);
        motors_.drive(MotorCommand::Right, 150);
        delay(150);
        motors_.stop();
    }
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::handleCanvasApi() {
    if (server_.hasArg("plain")) {
        String json = server_.arg("plain");
        JsonDocument doc;
        deserializeJson(doc, json);
        JsonArray array = doc.as<JsonArray>();
        app_.taskManager().clearCanvas();
        uint8_t i = 0;
        for (JsonVariant v : array) {
            uint8_t x = (i % 16) * 8;
            uint8_t y = (i / 16) * 4;
            bool on = v.as<int>() == 1;
            for (uint8_t dx = 0; dx < 8; ++dx) {
                for (uint8_t dy = 0; dy < 4; ++dy) {
                    app_.taskManager().setPixel(x + dx, y + dy, on);
                }
            }
            i++;
        }
        app_.setMode(AppMode::Canvas);
    }
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::handleGuardApi() {
    bool armed = server_.arg("armed") == "true";
    app_.taskManager().setGuardArmed(armed);
    if (armed) {
        app_.setMode(AppMode::DeskGuard);
    }
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::updateSettings() {
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::updateTime() {
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::updateWiFi() {
    const String ssid = server_.arg("ssid");
    const String password = server_.arg("password");
    if (ssid.length() > 0) {
        app_.setWiFiCredentials(ssid, password);
        restartRequested_ = true;
    }
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::motorCommand() {
    const String command = server_.arg("command");
    const int requestedSpeed = constrain(server_.arg("speed").toInt(), 0, 255);
    MotorCommand motorCommand = MotorCommand::Stop;
    if (command == "forward") motorCommand = MotorCommand::Forward;
    else if (command == "backward") motorCommand = MotorCommand::Backward;
    else if (command == "left") motorCommand = MotorCommand::Left;
    else if (command == "right") motorCommand = MotorCommand::Right;

    motors_.drive(motorCommand, static_cast<uint8_t>(requestedSpeed));
    app_.setMode(AppMode::RCCar);

    const unsigned long now = millis();
    if (motorCommand == MotorCommand::Forward) app_.setEmotion(Emotion::Excited, 0, now);
    else if (motorCommand == MotorCommand::Backward) app_.setEmotion(Emotion::Sad, 0, now);
    else if (motorCommand == MotorCommand::Left || motorCommand == MotorCommand::Right) app_.setEmotion(Emotion::Cool, 0, now);

    server_.send(200, "application/json", "{\"ok\":true}");
}
