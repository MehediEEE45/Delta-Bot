#include "AppController.h"
#include "Config.h"
#include <WiFi.h>
#include <time.h>

namespace {
inline bool reached(unsigned long now, unsigned long deadline) {
    return static_cast<long>(now - deadline) >= 0;
}

constexpr AppMode BROWSABLE_MODES[] = {
    AppMode::Face,
    AppMode::TimeDate,
    AppMode::Weather,
    AppMode::Tasks,
    AppMode::Pomodoro,
    AppMode::Pet,
    AppMode::Quotes,
    AppMode::Music,
};
constexpr int BROWSABLE_COUNT = sizeof(BROWSABLE_MODES) / sizeof(BROWSABLE_MODES[0]);

constexpr Emotion RANDOM_EMOTIONS[] = {
    Emotion::Idle, Emotion::Happy, Emotion::Love, Emotion::Excited,
    Emotion::Cool, Emotion::Sad,   Emotion::Angry, Emotion::Surprised,
};
constexpr int RANDOM_EMOTION_COUNT = sizeof(RANDOM_EMOTIONS) / sizeof(RANDOM_EMOTIONS[0]);
}

AppController::AppController(FaceRenderer& renderer, WeatherService& weather)
    : renderer_(renderer), weather_(weather) {}

void AppController::begin() {
    settings_.begin();
    taskManager_.begin();
    // Apply what was loaded from flash immediately, not only on the next web
    // save -- otherwise a saved format/location sits unused until re-saved.
    renderer_.setClockFormat(settings_.use24Hour());
    weather_.setLocation(settings_.weatherLatitude(), settings_.weatherLongitude());
    mode_ = settings_.defaultMode();
    preNightMode_ = mode_;
    emotion_ = restingEmotion();
    lastInteractionAt_ = millis();
    nextIdleEmotionAt_ = lastInteractionAt_ + Config::IDLE_SURPRISE_INTERVAL_MS;
    scheduleRandomEmotion(lastInteractionAt_);
}

Emotion AppController::restingEmotion() const {
    if (mode_ == AppMode::Music) return Emotion::Happy;
    // A timed Happy/Love pulse from a feed or a pat already lands on the pet
    // screen via setEmotion(); once that pulse lapses, this is what takes
    // over -- so a neglected pet drifts to Sad/Angry with no extra timer.
    if (mode_ == AppMode::Pet) return taskManager_.petMoodEmotion();
    return Emotion::Idle;
}

void AppController::noteInteraction(unsigned long now) {
    lastInteractionAt_ = now;
    nextIdleEmotionAt_ = now + Config::IDLE_SURPRISE_INTERVAL_MS;
    scheduleRandomEmotion(now);
}

void AppController::scheduleRandomEmotion(unsigned long now) {
    nextRandomEmotionAt_ = now + static_cast<unsigned long>(
        random(Config::FACE_RANDOM_MIN_MS, Config::FACE_RANDOM_MAX_MS));
}

Emotion AppController::randomEmotionOtherThan(Emotion current) {
    int currentIndex = -1;
    for (int i = 0; i < RANDOM_EMOTION_COUNT; ++i) {
        if (RANDOM_EMOTIONS[i] == current) {
            currentIndex = i;
            break;
        }
    }
    if (currentIndex < 0) return RANDOM_EMOTIONS[random(0, RANDOM_EMOTION_COUNT)];
    int index = static_cast<int>(random(0, RANDOM_EMOTION_COUNT - 1));
    if (index >= currentIndex) ++index;
    return RANDOM_EMOTIONS[index];
}

void AppController::cycleMode(int delta) {
    int index = -1;
    for (int i = 0; i < BROWSABLE_COUNT; ++i) {
        if (BROWSABLE_MODES[i] == mode_) {
            index = i;
            break;
        }
    }
    // Standing on a screen outside the walk (a reminder, the guard alarm, RC)
    // steps onto the first entry going forwards and the last going backwards.
    const int next = index < 0
        ? (delta > 0 ? 0 : BROWSABLE_COUNT - 1)
        : (index + delta + BROWSABLE_COUNT) % BROWSABLE_COUNT;
    setMode(BROWSABLE_MODES[next]);
}

void AppController::update(unsigned long now) {
    settings_.updateBattery(now);
    taskManager_.updatePet(now);
    taskManager_.updatePomodoro(now);
    taskManager_.maybePersist(now);

    // Weather refreshes on its own cadence regardless of the visible mode, so
    // switching to the weather screen shows data that is already there.
    weather_.update(now);

    if (taskManager_.isGuardAlarmTriggered()) {
        mode_ = AppMode::DeskGuard;
        emotion_ = Emotion::Angry;
    }

    checkNightMode();
    checkReminder(now);

    const bool sleepSuppressed = mode_ == AppMode::Night || mode_ == AppMode::DeskGuard;
    if (emotion_ != Emotion::Sleep && !sleepSuppressed &&
        now - lastInteractionAt_ >= Config::IDLE_SLEEP_MS) {
        emotion_ = Emotion::Sleep;
        emotionTimed_ = false;
    }

    if (mode_ == AppMode::Music && emotion_ == Emotion::Happy && reached(now, nextIdleEmotionAt_)) {
        emotion_ = Emotion::Surprised;
        emotionTimed_ = true;
        emotionUntil_ = now + Config::IDLE_SURPRISE_DURATION_MS;
        nextIdleEmotionAt_ = now + Config::IDLE_SURPRISE_INTERVAL_MS;
    }

    if (emotionTimed_ && reached(now, emotionUntil_)) {
        emotionTimed_ = false;
        emotion_ = restingEmotion();
    }

    if (mode_ == AppMode::Face && !emotionTimed_ && emotion_ != Emotion::Sleep &&
        reached(now, nextRandomEmotionAt_)) {
        emotion_ = randomEmotionOtherThan(emotion_);
        scheduleRandomEmotion(now);
    }

    if (mode_ == AppMode::Music) {
        musicMode_.update(now, emotion_, renderer_, weather_.data(), WiFi.status() == WL_CONNECTED);
    } else {
        clockMode_.update(mode_, now, emotion_, renderer_, weather_.data(), WiFi.status() == WL_CONNECTED, &taskManager_);
    }
}

void AppController::checkNightMode() {
    const time_t rawTime = time(nullptr);
    if (rawTime <= 100000) return; // clock not synced yet

    struct tm timeInfo;
    if (localtime_r(&rawTime, &timeInfo) == nullptr) return;

    const int hour = timeInfo.tm_hour;
    const bool night = hour >= Config::NIGHT_START_HOUR || hour < Config::NIGHT_END_HOUR;

    if (!nightInitialised_) {
        nightInitialised_ = true;
        nightActive_ = night;
        if (!night) return;
    } else if (night == nightActive_) {
        return; // no edge; leave whatever mode the user chose alone
    } else {
        nightActive_ = night;
    }

    if (mode_ == AppMode::RCCar) return; // never interrupt someone driving

    if (night) {
        preNightMode_ = mode_;
        mode_ = AppMode::Night;
        emotion_ = Emotion::Sleep;
    } else {
        mode_ = preNightMode_ == AppMode::Night ? AppMode::TimeDate : preNightMode_;
        emotion_ = restingEmotion();
    }
    emotionTimed_ = false;
}

void AppController::checkReminder(unsigned long now) {
    if (!taskManager_.consumeReminderTrigger()) return;
    mode_ = AppMode::Reminder;
    setEmotion(Emotion::Surprised, 30000, now);
    noteInteraction(now);
}

void AppController::handleTouch(TouchEvent event, unsigned long now) {
    if (event == TouchEvent::None) return;
    noteInteraction(now);

    if (taskManager_.isGuardArmed()) {
        taskManager_.triggerGuardAlarm();
        emotion_ = Emotion::Angry;
        emotionTimed_ = false;
        mode_ = AppMode::DeskGuard;
        return;
    }

    // A due reminder is acknowledged by any touch.
    if (mode_ == AppMode::Reminder && taskManager_.isReminderActive()) {
        taskManager_.clearReminder();
    }

    if (emotion_ == Emotion::Sleep && event != TouchEvent::LongPress) {
        emotion_ = restingEmotion();
        emotionTimed_ = false;
        return;
    }

    if (event == TouchEvent::SingleTap) {
        cycleMode(1);
    } else if (event == TouchEvent::DoubleTap) {
        // Untimed, so the expression holds until the next random change or the
        // next touch rather than snapping back after a few seconds.
        setEmotion(randomEmotionOtherThan(emotion_), 0, now);
        // A head pat cheers the pet up, matching the web "Pet Head" action.
        if (mode_ == AppMode::Pet) taskManager_.petPet();
    } else if (event == TouchEvent::TripleTap) {
        cycleMode(-1);
    } else if (event == TouchEvent::LongPress) {
        setEmotion(Emotion::Sleep, 0, now);
    }
}

void AppController::setMode(AppMode mode) {
    const unsigned long now = millis();
    mode_ = mode;
    // Choosing a mode is an interaction, and it overrides the night latch until
    // the next 22:00 / 06:00 edge.
    preNightMode_ = mode;
    noteInteraction(now);
    emotion_ = restingEmotion();
    emotionTimed_ = false;
    emotionUntil_ = 0;
}

void AppController::setEmotion(Emotion emotion, unsigned long durationMs, unsigned long now) {
    noteInteraction(now);
    emotion_ = emotion;
    emotionTimed_ = durationMs != 0;
    emotionUntil_ = emotionTimed_ ? now + durationMs : 0;
}

AppMode AppController::mode() const { return mode_; }
Emotion AppController::emotion() const { return emotion_; }
bool AppController::nightActive() const { return nightActive_; }
void AppController::setGaze(float x, float y) { renderer_.animator().lookAt(x, y); }
void AppController::clearGaze() { renderer_.animator().releaseLook(); }
const WeatherData& AppController::weatherData() const { return weather_.data(); }
void AppController::requestWeatherRefresh() { weather_.requestRefresh(); }
void AppController::setDefaultMode(AppMode mode) { settings_.setDefaultMode(mode); }
AppMode AppController::defaultMode() const { return settings_.defaultMode(); }
const char* AppController::defaultModeName() const { return appModeName(settings_.defaultMode()); }
uint8_t AppController::batteryPercent() const { return settings_.batteryPercent(); }
const char* AppController::wifiSsid() const { return settings_.wifiSsid(); }
const char* AppController::wifiPassword() const { return settings_.wifiPassword(); }
void AppController::setWiFiCredentials(const String& ssid, const String& password) { settings_.setWiFiCredentials(ssid, password); }
const char* AppController::timezone() const { return settings_.timezone(); }
bool AppController::use24Hour() const { return settings_.use24Hour(); }
void AppController::setClockSettings(const String& timezone, bool use24Hour) {
    settings_.setClockSettings(timezone, use24Hour);
    renderer_.setClockFormat(use24Hour);
}
const char* AppController::weatherLatitude() const { return settings_.weatherLatitude(); }
const char* AppController::weatherLongitude() const { return settings_.weatherLongitude(); }
void AppController::setWeatherLocation(const String& latitude, const String& longitude) {
    settings_.setWeatherLocation(latitude, longitude);
    weather_.setLocation(latitude, longitude);
    weather_.requestRefresh();
}
const char* AppController::modeName() const { return appModeName(mode_); }

TaskManager& AppController::taskManager() {
    return taskManager_;
}

const char* AppController::emotionName() const {
    switch (emotion_) {
        case Emotion::Happy: return "happy";
        case Emotion::Love: return "love";
        case Emotion::Excited: return "excited";
        case Emotion::Cool: return "cool";
        case Emotion::Sad: return "sad";
        case Emotion::Angry: return "angry";
        case Emotion::Sleep: return "sleep";
        case Emotion::Surprised: return "surprised";
        default: return "idle";
    }
}
