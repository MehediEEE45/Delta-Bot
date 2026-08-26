#include "WebController.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <sys/time.h>
#include "Config.h"

namespace {
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Delta-Bot Control Center</title><style>
:root{--ink:#172026;--paper:#f4efe6;--accent:#e46b3e;--line:#cbbfaf;--blue:#087bea;--green:#0a9b1d}*{box-sizing:border-box}body{font-family:system-ui,sans-serif;background:var(--paper);color:var(--ink);max-width:720px;margin:0 auto;padding:16px}header{border-bottom:3px solid var(--ink);display:flex;justify-content:space-between;align-items:center;padding-bottom:12px}h1{font-size:26px;margin:0}.panel{border:1px solid var(--line);padding:14px;margin-top:14px;background:#fffaf2;border-radius:6px}h2{font-size:14px;text-transform:uppercase;letter-spacing:1px;margin:0 0 10px;color:var(--accent)}button{background:var(--ink);color:#fff;border:0;padding:8px 12px;margin:3px;border-radius:4px;font-size:13px;cursor:pointer}button:hover,.active{background:var(--accent)}input{padding:8px;margin:4px 0;border:1px solid var(--line);border-radius:4px;width:100%}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:6px}.driveGrid{display:grid;grid-template-columns:repeat(3,60px);gap:6px;justify-content:center;margin:10px auto}.driveBtn{background:var(--blue);height:50px;font-size:12px;border-radius:6px}.canvasGrid{display:grid;grid-template-columns:repeat(16,1fr);gap:1px;background:#ccc;padding:2px;max-width:320px;margin:10px auto}.cell{background:#fff;aspect-ratio:1;cursor:pointer}.cell.on{background:#000}.row{display:flex;align-items:center;gap:8px;border-bottom:1px solid var(--line);padding:6px 0}.row span{flex:1}.row .done{text-decoration:line-through;opacity:.55}.mini{padding:4px 8px;font-size:12px;margin:0}.err{color:#b00;font-size:12px;min-height:16px}
</style></head><body>
<header><h1>&#129302; Delta-Bot</h1><small id="clock">--:--:--</small></header>

<div class="panel"><h2>Live Status</h2><div id="status" style="font-family:monospace;white-space:pre-line">Connecting...</div><div class="err" id="err"></div><button onclick="refresh()">Refresh</button></div>

<div class="panel"><h2>RC Car Drive</h2>
<div style="text-align:center">SPEED <input id="motorSpeed" type="range" min="0" max="255" value="180"></div>
<div class="driveGrid">
<span></span><button class="driveBtn" onpointerdown="motor('forward')" onpointerup="releaseMotor()" onpointercancel="releaseMotor()" onpointerleave="releaseMotor()">&#9650; FW</button><span></span>
<button class="driveBtn" onpointerdown="motor('left')" onpointerup="releaseMotor()" onpointercancel="releaseMotor()" onpointerleave="releaseMotor()">&#9668; LT</button>
<button class="driveBtn" style="background:#555" onclick="releaseMotor()">STOP</button>
<button class="driveBtn" onpointerdown="motor('right')" onpointerup="releaseMotor()" onpointercancel="releaseMotor()" onpointerleave="releaseMotor()">RT &#9658;</button>
<span></span><button class="driveBtn" onpointerdown="motor('backward')" onpointerup="releaseMotor()" onpointercancel="releaseMotor()" onpointerleave="releaseMotor()">&#9660; BW</button><span></span>
</div></div>

<div class="panel"><h2>Web Voice Command</h2>
<button onclick="startVoice()" style="background:var(--green)">Speak Command</button><span id="voiceText"></span></div>

<div class="panel"><h2>Emotions &amp; Modes</h2>
<div class="grid">
<button onclick="send('happy')">Happy</button><button onclick="send('love')">Love</button><button onclick="send('angry')">Angry</button>
<button onclick="send('cool')">Cool</button><button onclick="send('surprised')">Surprise</button><button onclick="send('sleep')">Sleep</button>
<button onclick="send('time')">Time</button><button onclick="send('weather')">Weather</button><button onclick="send('music')">Music</button>
<button onclick="send('tasks')">Tasks</button><button onclick="send('notice')">Notice</button><button onclick="send('pomodoro')">Pomodoro</button>
<button onclick="send('quotes')">Quotes</button><button onclick="send('pet')">Pet Mode</button><button onclick="send('decision')">8-Ball</button>
<button onclick="send('reminder')">Reminder</button><button onclick="send('canvas')">Canvas</button><button onclick="send('night')">Night</button>
<button onclick="send('face')">Face</button>
</div>
<div style="margin-top:8px">Boot into: <button onclick="setDefault()">Save current mode as default</button> <span id="defMode"></span></div></div>

<div class="panel"><h2>Virtual Pet Station</h2>
<button onclick="petAction('feed')">Feed Pizza</button><button onclick="petAction('pet')">Pet Head</button>
<div id="petStats" style="font-family:monospace"></div></div>

<div class="panel"><h2>Magic 8-Ball Decision Maker</h2>
<input id="askText" placeholder="Ask a YES/NO question..."><button onclick="askQuestion()">Ask Delta-Bot</button>
<div id="answer" style="font-weight:bold"></div></div>

<div class="panel"><h2>Task Manager</h2>
<input id="taskText" placeholder="New task..." maxlength="60"><button onclick="addTask()">Add Task</button>
<div id="taskList"></div></div>

<div class="panel"><h2>Broadcast Notice</h2>
<input id="noticeMsg" placeholder="Notice message..." maxlength="120"><button onclick="sendNotice()">Send Notice</button></div>

<div class="panel"><h2>Reminder Alarm</h2>
<input id="remTitle" placeholder="Remind me to..." maxlength="60">
<input id="remMins" type="number" min="1" max="1440" value="10" placeholder="Minutes from now">
<button onclick="setReminder()">Set Reminder</button><button onclick="clearReminder()">Clear</button>
<div id="remStatus" style="font-family:monospace"></div></div>

<div class="panel"><h2>Pomodoro Timer</h2>
<button onclick="pomoAction('start')">Start / Resume</button><button onclick="pomoAction('pause')">Pause</button><button onclick="pomoAction('reset')">Reset</button>
<div id="pomoStatus" style="font-family:monospace"></div></div>

<div class="panel"><h2>Daily Quote</h2><button onclick="newQuote()">New Quote</button><span id="quote"></span></div>

<div class="panel"><h2>Pixel Art Canvas</h2>
<div class="canvasGrid" id="canvasGrid"></div><button onclick="clearCanvas()">Clear Canvas</button></div>

<div class="panel"><h2>Desk Guard Security</h2>
<button onclick="toggleGuard(true)">Arm Guard</button><button onclick="toggleGuard(false)">Disarm</button></div>

<div class="panel"><h2>Wi-Fi Setup</h2>
<input id="wifiSsid" placeholder="SSID"><input id="wifiPassword" type="password" placeholder="Password"><button onclick="saveWiFi()">Save &amp; Reboot</button></div>

<script>
let motorTimer=0,pixels=new Array(256).fill(0),canvasTimer=0;
const grid=document.getElementById('canvasGrid');
for(let i=0;i<256;i++){let c=document.createElement('div');c.className='cell';c.onclick=()=>{pixels[i]=pixels[i]?0:1;c.classList.toggle('on',!!pixels[i]);queueCanvas()};grid.appendChild(c)}
function showErr(m){document.getElementById('err').textContent=m||''}
async function post(u,o){try{const r=await fetch(u,Object.assign({method:'POST'},o||{}));if(r.status===401){showErr('Unauthorized - reload and enter the web password.');return null}if(!r.ok){showErr('Request failed: '+r.status);return null}showErr('');return r}catch(e){showErr('Network error');return null}}
async function send(c){await post('/api/command?value='+encodeURIComponent(c));refresh()}
function motor(c){clearInterval(motorTimer);const s=document.getElementById('motorSpeed').value;const fire=()=>fetch('/api/motor?command='+c+'&speed='+document.getElementById('motorSpeed').value,{method:'POST'});fire();if(c!=='stop')motorTimer=setInterval(fire,250)}
function releaseMotor(){clearInterval(motorTimer);motorTimer=0;fetch('/api/motor?command=stop&speed=0',{method:'POST'})}
async function addTask(){const t=document.getElementById('taskText').value.trim();if(!t)return;await post('/api/tasks?action=add&text='+encodeURIComponent(t));document.getElementById('taskText').value='';refresh()}
async function toggleTask(i){await post('/api/tasks?action=toggle&index='+i);refresh()}
async function deleteTask(i){await post('/api/tasks?action=delete&index='+i);refresh()}
async function sendNotice(){const m=document.getElementById('noticeMsg').value.trim();if(!m)return;await post('/api/notice?text='+encodeURIComponent(m));refresh()}
async function setReminder(){const t=document.getElementById('remTitle').value.trim();const m=document.getElementById('remMins').value;if(!t||!(m>0))return;await post('/api/reminder?action=set&title='+encodeURIComponent(t)+'&minutes='+m);refresh()}
async function clearReminder(){await post('/api/reminder?action=clear');refresh()}
async function pomoAction(a){await post('/api/pomodoro?action='+a);refresh()}
async function petAction(a){await post('/api/pet?action='+a);refresh()}
async function newQuote(){await post('/api/quote');refresh()}
async function setDefault(){await post('/api/default-mode');refresh()}
async function askQuestion(){const q=document.getElementById('askText').value.trim();if(!q)return;await post('/api/decision?q='+encodeURIComponent(q));refresh()}
async function toggleGuard(a){await post('/api/guard?armed='+a);refresh()}
function queueCanvas(){clearTimeout(canvasTimer);canvasTimer=setTimeout(sendCanvas,120)}
async function sendCanvas(){await post('/api/canvas',{headers:{'Content-Type':'application/json'},body:JSON.stringify(pixels)})}
async function clearCanvas(){pixels.fill(0);document.querySelectorAll('.cell').forEach(c=>c.classList.remove('on'));sendCanvas()}
async function saveWiFi(){const s=document.getElementById('wifiSsid').value;const p=document.getElementById('wifiPassword').value;if(!s)return;if(!confirm('Save Wi-Fi settings and reboot Delta-Bot?'))return;await post('/api/wifi?ssid='+encodeURIComponent(s)+'&password='+encodeURIComponent(p))}
function startVoice(){const SR=window.SpeechRecognition||window.webkitSpeechRecognition;if(!SR){alert('Speech Recognition is not supported in this browser.');return}const r=new SR();r.onresult=e=>{const t=e.results[0][0].transcript.toLowerCase();document.getElementById('voiceText').textContent=' Heard: "'+t+'"';if(t.includes('forward'))motor('forward');else if(t.includes('back'))motor('backward');else if(t.includes('left'))motor('left');else if(t.includes('right'))motor('right');else if(t.includes('stop'))releaseMotor();else if(t.includes('happy'))send('happy');else if(t.includes('sleep'))send('sleep');else if(t.includes('weather'))send('weather')};r.start()}
function renderTasks(list){const el=document.getElementById('taskList');el.innerHTML='';(list||[]).forEach((t,i)=>{const row=document.createElement('div');row.className='row';const s=document.createElement('span');s.textContent=t.text;if(t.done)s.className='done';const b1=document.createElement('button');b1.className='mini';b1.textContent=t.done?'Undo':'Done';b1.onclick=()=>toggleTask(i);const b2=document.createElement('button');b2.className='mini';b2.textContent='Delete';b2.onclick=()=>deleteTask(i);row.append(s,b1,b2);el.appendChild(row)})}
async function refresh(){try{const r=await fetch('/api/status');if(r.status===401){showErr('Unauthorized - reload and enter the web password.');return}const s=await r.json();showErr('');
document.getElementById('status').textContent='MODE: '+s.mode+' | FACE: '+s.emotion+' | BATT: '+(s.batteryMeasured?s.battery+'%':'n/a')+' | NET: '+s.network+' | WEATHER: '+s.weather.status+(s.weather.valid?' '+s.weather.temperature+'C':'');
document.getElementById('clock').textContent=s.time;
document.getElementById('defMode').textContent='(currently: '+s.defaultMode+')';
document.getElementById('petStats').textContent='Hunger '+s.pet.hunger+' / Happy '+s.pet.happiness;
document.getElementById('answer').textContent=s.lastAnswer||'';
document.getElementById('quote').textContent=' '+s.quote;
document.getElementById('pomoStatus').textContent=s.pomodoro.state==='stopped'?'Stopped':s.pomodoro.state+' - '+String(Math.floor(s.pomodoro.remaining/60)).padStart(2,'0')+':'+String(s.pomodoro.remaining%60).padStart(2,'0');
document.getElementById('remStatus').textContent=s.reminder.active?(s.reminder.title+(s.reminder.due?' (DUE)':' - pending')):'No reminder set';
renderTasks(s.tasks)}catch(e){showErr('Network error')}}
setInterval(refresh,2000);refresh();
</script></body></html>
)rawliteral";

// Guard against a malicious or buggy client sending a huge canvas payload.
constexpr size_t MAX_CANVAS_BODY = 4096;
}

WebController::WebController(AppController& app, MotorController& motors) : app_(app), motors_(motors) {}

void WebController::begin() {
    registerRoutes();
    server_.begin();
}

void WebController::update() {
    server_.handleClient();
    // Deferred so the 200 for /api/wifi is actually flushed to the browser
    // before the chip resets, and without blocking the loop in a delay().
    if (restartRequested_ && millis() - restartAt_ >= 500) {
        Serial.println("Restarting to apply new Wi-Fi settings.");
        ESP.restart();
    }
}

bool WebController::requireAuth() {
    // An empty password disables auth; anything else challenges every request.
    if (strlen(Config::WEB_PASSWORD) == 0) return true;
    if (server_.authenticate(Config::WEB_USER, Config::WEB_PASSWORD)) return true;
    server_.requestAuthentication(BASIC_AUTH, "Delta-Bot");
    return false;
}

void WebController::sendOk() {
    server_.send(200, "application/json", "{\"ok\":true}");
}

void WebController::sendError(int code, const char* message) {
    JsonDocument doc;
    doc["ok"] = false;
    doc["error"] = message;
    String out;
    serializeJson(doc, out);
    server_.send(code, "application/json", out);
}

void WebController::registerRoutes() {
    server_.on("/", HTTP_GET, [this]() {
        if (!requireAuth()) return;
        server_.send_P(200, "text/html", INDEX_HTML);
    });
    server_.on("/api/status", HTTP_GET, [this]() { if (requireAuth()) sendStatus(); });
    server_.on("/api/weather/refresh", HTTP_POST, [this]() { if (requireAuth()) refreshWeather(); });
    server_.on("/api/wifi", HTTP_POST, [this]() { if (requireAuth()) updateWiFi(); });
    server_.on("/api/motor", HTTP_POST, [this]() { if (requireAuth()) motorCommand(); });

    server_.on("/api/command", HTTP_POST, [this]() {
        if (!requireAuth()) return;
        if (handleCommand(server_.arg("value"))) sendOk();
        else sendError(400, "unknown command");
    });

    server_.on("/api/tasks", HTTP_POST, [this]() { if (requireAuth()) handleTasksApi(); });
    server_.on("/api/notice", HTTP_POST, [this]() { if (requireAuth()) handleNoticeApi(); });
    server_.on("/api/reminder", HTTP_POST, [this]() { if (requireAuth()) handleReminderApi(); });
    server_.on("/api/pomodoro", HTTP_POST, [this]() { if (requireAuth()) handlePomodoroApi(); });
    server_.on("/api/pet", HTTP_POST, [this]() { if (requireAuth()) handlePetApi(); });
    server_.on("/api/decision", HTTP_POST, [this]() { if (requireAuth()) handleDecisionApi(); });
    server_.on("/api/canvas", HTTP_POST, [this]() { if (requireAuth()) handleCanvasApi(); });
    server_.on("/api/guard", HTTP_POST, [this]() { if (requireAuth()) handleGuardApi(); });
    server_.on("/api/quote", HTTP_POST, [this]() { if (requireAuth()) handleQuoteApi(); });
    server_.on("/api/default-mode", HTTP_POST, [this]() { if (requireAuth()) handleDefaultModeApi(); });

    server_.onNotFound([this]() { sendError(404, "no such endpoint"); });
}

void WebController::sendStatus() {
    const unsigned long now = millis();
    TaskManager& tasks = app_.taskManager();

    JsonDocument document;
    document["mode"] = app_.modeName();
    document["emotion"] = app_.emotionName();
    document["motor"] = motors_.commandName();
    document["defaultMode"] = app_.defaultModeName();
    document["battery"] = app_.batteryPercent();
    document["batteryMeasured"] = Config::BATTERY_ADC_PIN != 255;
    document["night"] = app_.nightActive();
    document["network"] = WiFi.status() == WL_CONNECTED ? "online" : "offline";
    document["ip"] = WiFi.getMode() == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();

    const time_t currentTime = time(nullptr);
    if (currentTime > 100000) {
        struct tm timeInfo;
        char stamp[32] = "SYNCING";
        if (localtime_r(&currentTime, &timeInfo) != nullptr) {
            strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &timeInfo);
        }
        document["time"] = stamp;
    } else {
        document["time"] = "SYNCING";
    }

    const WeatherData& weather = app_.weatherData();
    JsonObject weatherObj = document["weather"].to<JsonObject>();
    weatherObj["valid"] = weather.valid;
    weatherObj["temperature"] = weather.valid ? weather.temperature : 0.0f;
    weatherObj["code"] = weather.weatherCode;
    weatherObj["status"] = weather.requesting ? "requesting" : (weather.valid ? "ok" : "offline");

    JsonArray taskArray = document["tasks"].to<JsonArray>();
    for (size_t i = 0; i < tasks.taskCount(); ++i) {
        const TaskItem* item = tasks.getTask(i);
        if (!item) continue;
        JsonObject entry = taskArray.add<JsonObject>();
        entry["text"] = item->text;
        entry["done"] = item->completed;
    }

    JsonObject pet = document["pet"].to<JsonObject>();
    pet["hunger"] = tasks.petStats().hunger;
    pet["happiness"] = tasks.petStats().happiness;

    JsonObject pomo = document["pomodoro"].to<JsonObject>();
    pomo["state"] = tasks.pomodoroStateName();
    pomo["remaining"] = tasks.pomodoroRemainingSec(now);

    JsonObject reminder = document["reminder"].to<JsonObject>();
    reminder["active"] = tasks.isReminderActive();
    reminder["title"] = tasks.reminderTitle();
    reminder["due"] = tasks.isReminderDue();

    JsonObject guard = document["guard"].to<JsonObject>();
    guard["armed"] = tasks.isGuardArmed();
    guard["alarm"] = tasks.isGuardAlarmTriggered();

    document["notice"] = tasks.notice();
    document["quote"] = tasks.currentQuote();
    document["lastAnswer"] = tasks.lastAnswer();

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

    AppMode mode;
    if (appModeFromName(command, mode)) {
        app_.setMode(mode);
        return true;
    }

    if (command == "happy") app_.setEmotion(Emotion::Happy, 0, now);
    else if (command == "love") app_.setEmotion(Emotion::Love, 0, now);
    else if (command == "angry") app_.setEmotion(Emotion::Angry, 0, now);
    else if (command == "cool") app_.setEmotion(Emotion::Cool, 0, now);
    else if (command == "excited") app_.setEmotion(Emotion::Excited, 0, now);
    else if (command == "sad") app_.setEmotion(Emotion::Sad, 0, now);
    else if (command == "surprised") app_.setEmotion(Emotion::Surprised, 5000, now);
    else if (command == "sleep") app_.setEmotion(Emotion::Sleep, 0, now);
    else return false;
    return true;
}

void WebController::handleTasksApi() {
    const String action = server_.arg("action");
    TaskManager& tasks = app_.taskManager();

    if (action == "toggle" || action == "delete") {
        if (!server_.hasArg("index")) { sendError(400, "index required"); return; }
        const long index = server_.arg("index").toInt();
        if (index < 0) { sendError(400, "index out of range"); return; }
        const bool ok = action == "toggle"
            ? tasks.toggleTask(static_cast<size_t>(index))
            : tasks.deleteTask(static_cast<size_t>(index));
        if (!ok) { sendError(404, "no such task"); return; }
        app_.setMode(AppMode::Tasks);
        sendOk();
        return;
    }

    if (action == "clear") {
        tasks.clearTasks();
        app_.setMode(AppMode::Tasks);
        sendOk();
        return;
    }

    // Default action: add.
    const String text = server_.arg("text");
    if (text.length() == 0) { sendError(400, "text required"); return; }
    if (!tasks.addTask(text)) { sendError(409, "task list is full"); return; }
    app_.setMode(AppMode::Tasks);
    sendOk();
}

void WebController::handleNoticeApi() {
    const String text = server_.arg("text");
    if (text.length() == 0) { sendError(400, "text required"); return; }
    app_.taskManager().setNotice(text);
    app_.setMode(AppMode::Notice);
    sendOk();
}

void WebController::handleReminderApi() {
    if (server_.arg("action") == "clear") {
        app_.taskManager().clearReminder();
        sendOk();
        return;
    }

    const String title = server_.arg("title");
    const long minutes = server_.arg("minutes").toInt();
    if (title.length() == 0) { sendError(400, "title required"); return; }
    if (minutes <= 0 || minutes > 1440) { sendError(400, "minutes must be 1-1440"); return; }

    const time_t nowSec = time(nullptr);
    if (nowSec <= 100000) { sendError(503, "clock not synced yet"); return; }

    app_.taskManager().setReminder(title, static_cast<unsigned long>(nowSec + minutes * 60));
    app_.setMode(AppMode::Reminder);
    sendOk();
}

void WebController::handlePomodoroApi() {
    const String action = server_.arg("action");
    const unsigned long now = millis();

    if (action == "start") {
        app_.taskManager().startPomodoro(now);
        app_.setMode(AppMode::Pomodoro);
    } else if (action == "pause") {
        app_.taskManager().pausePomodoro(now);
    } else if (action == "reset") {
        app_.taskManager().resetPomodoro();
    } else {
        sendError(400, "action must be start, pause or reset");
        return;
    }
    sendOk();
}

void WebController::handlePetApi() {
    const String action = server_.arg("action");
    if (action == "feed") {
        app_.taskManager().feedPet();
        app_.setEmotion(Emotion::Happy, 3000, millis());
    } else if (action == "pet") {
        app_.taskManager().petPet();
        app_.setEmotion(Emotion::Love, 3000, millis());
    } else {
        sendError(400, "action must be feed or pet");
        return;
    }
    app_.setMode(AppMode::Pet);
    sendOk();
}

void WebController::handleDecisionApi() {
    const String question = server_.arg("q");
    if (question.length() == 0) { sendError(400, "question required"); return; }

    app_.taskManager().askDecision(question);
    app_.setMode(AppMode::Decision);
    // Non-blocking: MotorController::update() drives the wiggle from the loop.
    motors_.startShake(millis());
    sendOk();
}

void WebController::handleQuoteApi() {
    app_.taskManager().randomQuote();
    app_.setMode(AppMode::Quotes);
    sendOk();
}

void WebController::handleDefaultModeApi() {
    // Persist whichever mode is showing right now as the boot mode.
    app_.setDefaultMode(app_.mode());
    sendOk();
}

void WebController::handleCanvasApi() {
    if (!server_.hasArg("plain")) { sendError(400, "body required"); return; }

    const String json = server_.arg("plain");
    if (json.length() > MAX_CANVAS_BODY) { sendError(413, "payload too large"); return; }

    JsonDocument doc;
    if (deserializeJson(doc, json)) { sendError(400, "invalid json"); return; }

    JsonArray array = doc.as<JsonArray>();
    if (array.isNull() || array.size() != Config::CANVAS_CELL_COUNT) {
        sendError(400, "expected an array of 256 cells");
        return;
    }

    // Only clear once the payload is known good, so a malformed POST can no
    // longer wipe the drawing the user already made.
    TaskManager& tasks = app_.taskManager();
    tasks.clearCanvas();

    const uint8_t cellW = Config::CANVAS_WIDTH / Config::CANVAS_CELLS_X;  // 8
    const uint8_t cellH = Config::CANVAS_HEIGHT / Config::CANVAS_CELLS_Y; // 4

    size_t i = 0;
    for (JsonVariant v : array) {
        const bool on = v.as<int>() == 1;
        if (on) {
            const uint8_t x0 = static_cast<uint8_t>((i % Config::CANVAS_CELLS_X) * cellW);
            const uint8_t y0 = static_cast<uint8_t>((i / Config::CANVAS_CELLS_X) * cellH);
            for (uint8_t dx = 0; dx < cellW; ++dx) {
                for (uint8_t dy = 0; dy < cellH; ++dy) {
                    tasks.setPixel(x0 + dx, y0 + dy, true);
                }
            }
        }
        ++i;
    }

    app_.setMode(AppMode::Canvas);
    sendOk();
}

void WebController::handleGuardApi() {
    const String armedArg = server_.arg("armed");
    if (armedArg != "true" && armedArg != "false") {
        sendError(400, "armed must be true or false");
        return;
    }

    const bool armed = armedArg == "true";
    app_.taskManager().setGuardArmed(armed);
    if (armed) {
        app_.setMode(AppMode::DeskGuard);
    } else {
        app_.setMode(app_.defaultMode());
    }
    sendOk();
}

void WebController::updateWiFi() {
    const String ssid = server_.arg("ssid");
    const String password = server_.arg("password");
    if (ssid.length() == 0) { sendError(400, "ssid required"); return; }
    if (ssid.length() > 32 || password.length() > 63) { sendError(400, "credentials too long"); return; }

    app_.setWiFiCredentials(ssid, password);
    restartRequested_ = true;
    restartAt_ = millis();
    sendOk();
}

void WebController::motorCommand() {
    const String command = server_.arg("command");
    const int requestedSpeed = constrain(server_.arg("speed").toInt(), 0, 255);

    MotorCommand motorCommand = MotorCommand::Stop;
    if (command == "forward") motorCommand = MotorCommand::Forward;
    else if (command == "backward") motorCommand = MotorCommand::Backward;
    else if (command == "left") motorCommand = MotorCommand::Left;
    else if (command == "right") motorCommand = MotorCommand::Right;
    else if (command != "stop") { sendError(400, "unknown motor command"); return; }

    motors_.drive(motorCommand, static_cast<uint8_t>(requestedSpeed));

    const unsigned long now = millis();
    if (motorCommand == MotorCommand::Stop) {
        app_.clearGaze();
        sendOk();
        return;
    }

    app_.setMode(AppMode::RCCar);
    // Lean the gaze into the direction of travel: the "steering eyes".
    if (motorCommand == MotorCommand::Forward) {
        app_.setEmotion(Emotion::Excited, 0, now);
        app_.setGaze(0.0f, -0.6f);
    } else if (motorCommand == MotorCommand::Backward) {
        app_.setEmotion(Emotion::Sad, 0, now);
        app_.setGaze(0.0f, 0.7f);
    } else {
        app_.setEmotion(Emotion::Cool, 0, now);
        app_.setGaze(motorCommand == MotorCommand::Left ? -1.0f : 1.0f, 0.0f);
    }

    sendOk();
}
