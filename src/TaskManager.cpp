#include "TaskManager.h"
#include "Config.h"
#include <string.h>

namespace {
const char* const QUOTES[] = {
    "Believe you can and you're halfway there.",
    "Do what you can, with what you have.",
    "Every day is a fresh start.",
    "Stay hungry, stay foolish.",
    "Small steps every day lead to big results.",
    "Dream big, work hard, stay focused."
};
const size_t NUM_QUOTES = sizeof(QUOTES) / sizeof(QUOTES[0]);

const char* const ANSWERS[] = {
    "YES!",
    "NO!",
    "ABSOLUTELY!",
    "NEVER!",
    "TRY AGAIN!",
    "ASK LATER!",
    "PROBABLY!",
    "NO WAY!"
};
const size_t NUM_ANSWERS = sizeof(ANSWERS) / sizeof(ANSWERS[0]);

// Rollover-safe deadline test: true once `now` has reached `deadline`.
inline bool reached(unsigned long now, unsigned long deadline) {
    return static_cast<long>(now - deadline) >= 0;
}
}

TaskManager::TaskManager() {
    clearCanvas();
    currentQuote_ = QUOTES[0];
}

void TaskManager::begin() {
    loadFromFlash();
    lastSaveAt_ = millis();
}

void TaskManager::loadFromFlash() {
    prefs_.begin("task_mgr", true); // read-only mode
    taskCount_ = prefs_.getUChar("t_count", 0);
    if (taskCount_ > MAX_TASKS) taskCount_ = 0;

    for (size_t i = 0; i < taskCount_; ++i) {
        String keyT = "t_" + String(i);
        String keyC = "tc_" + String(i);
        tasks_[i].text = prefs_.getString(keyT.c_str(), "");
        tasks_[i].completed = prefs_.getBool(keyC.c_str(), false);
    }

    pet_.hunger = prefs_.getUChar("p_hunger", 80);
    pet_.happiness = prefs_.getUChar("p_happy", 90);
    prefs_.end();
}

void TaskManager::saveToFlash() {
    prefs_.begin("task_mgr", false); // read-write mode
    prefs_.putUChar("t_count", static_cast<uint8_t>(taskCount_));
    for (size_t i = 0; i < taskCount_; ++i) {
        String keyT = "t_" + String(i);
        String keyC = "tc_" + String(i);
        prefs_.putString(keyT.c_str(), tasks_[i].text);
        prefs_.putBool(keyC.c_str(), tasks_[i].completed);
    }
    prefs_.putUChar("p_hunger", pet_.hunger);
    prefs_.putUChar("p_happy", pet_.happiness);
    prefs_.end();
    dirty_ = false;
    lastSaveAt_ = millis();
}

void TaskManager::markDirty() {
    dirty_ = true;
}

void TaskManager::maybePersist(unsigned long now) {
    if (!dirty_) return;
    if (now - lastSaveAt_ < Config::FLASH_THROTTLE_MS) return;
    saveToFlash();
}

bool TaskManager::addTask(const String& text) {
    if (taskCount_ >= MAX_TASKS || text.length() == 0) return false;
    tasks_[taskCount_].text = text;
    tasks_[taskCount_].completed = false;
    taskCount_++;
    saveToFlash();
    return true;
}

bool TaskManager::toggleTask(size_t index) {
    if (index >= taskCount_) return false;
    tasks_[index].completed = !tasks_[index].completed;
    saveToFlash();
    return true;
}

bool TaskManager::deleteTask(size_t index) {
    if (index >= taskCount_) return false;
    for (size_t i = index; i + 1 < taskCount_; ++i) {
        tasks_[i] = tasks_[i + 1];
    }
    taskCount_--;
    tasks_[taskCount_].text = "";
    tasks_[taskCount_].completed = false;
    saveToFlash();
    return true;
}

void TaskManager::clearTasks() {
    for (size_t i = 0; i < MAX_TASKS; ++i) {
        tasks_[i].text = "";
        tasks_[i].completed = false;
    }
    taskCount_ = 0;
    saveToFlash();
}

size_t TaskManager::taskCount() const {
    return taskCount_;
}

const TaskItem* TaskManager::getTask(size_t index) const {
    if (index >= taskCount_) return nullptr;
    return &tasks_[index];
}

void TaskManager::setNotice(const String& notice, unsigned long durationMs) {
    notice_ = notice;
    noticeExpiresAt_ = millis() + durationMs;
}

String TaskManager::notice() const {
    return notice_;
}

bool TaskManager::hasActiveNotice(unsigned long now) const {
    return notice_.length() > 0 && !reached(now, noticeExpiresAt_);
}

void TaskManager::setReminder(const String& title, unsigned long targetTimeSec) {
    reminderTitle_ = title;
    reminderTargetTime_ = targetTimeSec;
    reminderActive_ = true;
    reminderFired_ = false;
}

String TaskManager::reminderTitle() const {
    return reminderTitle_;
}

unsigned long TaskManager::reminderTargetTime() const {
    return reminderTargetTime_;
}

bool TaskManager::isReminderActive() const {
    return reminderActive_;
}

bool TaskManager::isReminderDue() const {
    if (!reminderActive_) return false;
    const time_t current = time(nullptr);
    if (current <= 100000) return false; // clock has not synced yet
    return current >= static_cast<time_t>(reminderTargetTime_);
}

bool TaskManager::consumeReminderTrigger() {
    if (reminderFired_ || !isReminderDue()) return false;
    reminderFired_ = true;
    return true;
}

void TaskManager::clearReminder() {
    reminderActive_ = false;
    reminderFired_ = false;
    reminderTitle_ = "";
    reminderTargetTime_ = 0;
}

void TaskManager::startPomodoro(unsigned long now) {
    if (pomodoroState_ == PomodoroState::Paused) {
        // Resume: rebase the start so only the saved remainder plays out.
        pomodoroState_ = pomodoroDurationMs_ == Config::POMODORO_BREAK_MS
            ? PomodoroState::Break : PomodoroState::Work;
        pomodoroStartedAt_ = now - (pomodoroDurationMs_ - pomodoroPausedRemainingMs_);
        pomodoroPausedRemainingMs_ = 0;
        return;
    }
    if (pomodoroState_ != PomodoroState::Stopped) return;
    pomodoroState_ = PomodoroState::Work;
    pomodoroStartedAt_ = now;
    pomodoroDurationMs_ = Config::POMODORO_WORK_MS;
    pomodoroPausedRemainingMs_ = 0;
}

void TaskManager::pausePomodoro(unsigned long now) {
    if (pomodoroState_ != PomodoroState::Work && pomodoroState_ != PomodoroState::Break) return;
    const unsigned long elapsed = now - pomodoroStartedAt_;
    pomodoroPausedRemainingMs_ = elapsed >= pomodoroDurationMs_ ? 0 : pomodoroDurationMs_ - elapsed;
    pomodoroState_ = PomodoroState::Paused;
}

void TaskManager::resetPomodoro() {
    pomodoroState_ = PomodoroState::Stopped;
    pomodoroStartedAt_ = 0;
    pomodoroDurationMs_ = 0;
    pomodoroPausedRemainingMs_ = 0;
}

void TaskManager::updatePomodoro(unsigned long now) {
    if (pomodoroState_ != PomodoroState::Work && pomodoroState_ != PomodoroState::Break) return;
    if (now - pomodoroStartedAt_ < pomodoroDurationMs_) return;

    if (pomodoroState_ == PomodoroState::Work) {
        pomodoroState_ = PomodoroState::Break;
        pomodoroStartedAt_ = now;
        pomodoroDurationMs_ = Config::POMODORO_BREAK_MS;
    } else {
        resetPomodoro();
    }
}

PomodoroState TaskManager::pomodoroState() const {
    return pomodoroState_;
}

const char* TaskManager::pomodoroStateName() const {
    switch (pomodoroState_) {
        case PomodoroState::Work:   return "work";
        case PomodoroState::Break:  return "break";
        case PomodoroState::Paused: return "paused";
        default:                    return "stopped";
    }
}

unsigned long TaskManager::pomodoroRemainingSec(unsigned long now) const {
    if (pomodoroState_ == PomodoroState::Paused) return pomodoroPausedRemainingMs_ / 1000UL;
    if (pomodoroState_ == PomodoroState::Stopped) return 0;
    const unsigned long elapsed = now - pomodoroStartedAt_;
    if (elapsed >= pomodoroDurationMs_) return 0;
    return (pomodoroDurationMs_ - elapsed) / 1000UL;
}

float TaskManager::pomodoroProgress(unsigned long now) const {
    if (pomodoroDurationMs_ == 0) return 0.0f;
    unsigned long elapsed;
    if (pomodoroState_ == PomodoroState::Paused) {
        elapsed = pomodoroDurationMs_ - pomodoroPausedRemainingMs_;
    } else if (pomodoroState_ == PomodoroState::Stopped) {
        return 0.0f;
    } else {
        elapsed = now - pomodoroStartedAt_;
    }
    if (elapsed >= pomodoroDurationMs_) return 1.0f;
    return static_cast<float>(elapsed) / static_cast<float>(pomodoroDurationMs_);
}

void TaskManager::updatePet(unsigned long now) {
    if (pet_.lastDecayAt == 0) {
        pet_.lastDecayAt = now;
        return;
    }
    if (now - pet_.lastDecayAt < Config::PET_DECAY_INTERVAL_MS) return;

    pet_.lastDecayAt = now;
    if (pet_.hunger > 5) pet_.hunger -= 2;
    if (pet_.happiness > 5) pet_.happiness -= 1;
    // Background decay only flags the change; maybePersist() batches the write
    // so an idle bot does not commit to NVS once a minute forever.
    markDirty();
}

void TaskManager::feedPet() {
    pet_.hunger = min<uint8_t>(100, pet_.hunger + 25);
    pet_.happiness = min<uint8_t>(100, pet_.happiness + 10);
    saveToFlash();
}

void TaskManager::petPet() {
    pet_.happiness = min<uint8_t>(100, pet_.happiness + 20);
    saveToFlash();
}

const PetStats& TaskManager::petStats() const {
    return pet_;
}

String TaskManager::askDecision(const String& question) {
    (void)question; // the answer is deliberately independent of the question
    lastAnswer_ = ANSWERS[random(0, NUM_ANSWERS)];
    return lastAnswer_;
}

String TaskManager::lastAnswer() const {
    return lastAnswer_;
}

String TaskManager::randomQuote() {
    currentQuote_ = QUOTES[random(0, NUM_QUOTES)];
    return currentQuote_;
}

String TaskManager::currentQuote() const {
    return currentQuote_;
}

void TaskManager::clearCanvas() {
    memset(canvasBuffer_, 0, sizeof(canvasBuffer_));
}

void TaskManager::setPixel(uint8_t x, uint8_t y, bool color) {
    if (x >= Config::CANVAS_WIDTH || y >= Config::CANVAS_HEIGHT) return;
    // Adafruit_GFX drawBitmap layout: row-major, MSB-first within each byte.
    const size_t idx = static_cast<size_t>(y) * (Config::CANVAS_WIDTH / 8) + (x / 8);
    const uint8_t bit = 7 - (x % 8);
    if (color) {
        canvasBuffer_[idx] |= static_cast<uint8_t>(1u << bit);
    } else {
        canvasBuffer_[idx] &= static_cast<uint8_t>(~(1u << bit));
    }
}

const uint8_t* TaskManager::canvasBuffer() const {
    return canvasBuffer_;
}

void TaskManager::setGuardArmed(bool armed) {
    guardArmed_ = armed;
    if (!armed) {
        guardAlarmTriggered_ = false;
        guardPulseFired_ = false;
    }
}

bool TaskManager::isGuardArmed() const {
    return guardArmed_;
}

void TaskManager::triggerGuardAlarm() {
    if (guardArmed_) guardAlarmTriggered_ = true;
}

bool TaskManager::consumeGuardAlarmPulse() {
    if (!guardAlarmTriggered_ || guardPulseFired_) return false;
    guardPulseFired_ = true;
    return true;
}

bool TaskManager::isGuardAlarmTriggered() const {
    return guardAlarmTriggered_;
}

void TaskManager::resetGuardAlarm() {
    guardAlarmTriggered_ = false;
    guardPulseFired_ = false;
}
