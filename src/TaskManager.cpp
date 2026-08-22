#include "TaskManager.h"
#include "Config.h"

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
}

TaskManager::TaskManager() {
    clearCanvas();
    currentQuote_ = QUOTES[0];
}

void TaskManager::begin() {
    loadFromFlash();
}

void TaskManager::loadFromFlash() {
    prefs_.begin("task_mgr", true); // read-only mode
    taskCount_ = prefs_.getUChar("t_count", 0);
    if (taskCount_ > 5) taskCount_ = 0;

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
}

bool TaskManager::addTask(const String& text) {
    if (taskCount_ >= 5 || text.length() == 0) return false;
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
    for (size_t i = index; i < taskCount_ - 1; ++i) {
        tasks_[i] = tasks_[i + 1];
    }
    taskCount_--;
    saveToFlash();
    return true;
}

void TaskManager::clearTasks() {
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
    return notice_.length() > 0 && now < noticeExpiresAt_;
}

void TaskManager::setReminder(const String& title, unsigned long targetTimeSec) {
    reminderTitle_ = title;
    reminderTargetTime_ = targetTimeSec;
    reminderActive_ = true;
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

bool TaskManager::isReminderTriggered(unsigned long now) const {
    if (!reminderActive_) return false;
    time_t current = time(nullptr);
    return current >= (time_t)reminderTargetTime_;
}

void TaskManager::clearReminder() {
    reminderActive_ = false;
    reminderTitle_ = "";
    reminderTargetTime_ = 0;
}

void TaskManager::startPomodoro(unsigned long now) {
    if (pomodoroState_ == PomodoroState::Stopped) {
        pomodoroState_ = PomodoroState::Work;
        pomodoroStartedAt_ = now;
        pomodoroDurationMs_ = Config::POMODORO_WORK_MS;
    }
}

void TaskManager::pausePomodoro() {
    if (pomodoroState_ != PomodoroState::Stopped) {
        pomodoroState_ = PomodoroState::Stopped;
    }
}

void TaskManager::resetPomodoro() {
    pomodoroState_ = PomodoroState::Stopped;
    pomodoroStartedAt_ = 0;
}

void TaskManager::updatePomodoro(unsigned long now) {
    if (pomodoroState_ == PomodoroState::Stopped) return;
    if (now - pomodoroStartedAt_ >= pomodoroDurationMs_) {
        if (pomodoroState_ == PomodoroState::Work) {
            pomodoroState_ = PomodoroState::Break;
            pomodoroStartedAt_ = now;
            pomodoroDurationMs_ = Config::POMODORO_BREAK_MS;
        } else {
            pomodoroState_ = PomodoroState::Stopped;
        }
    }
}

PomodoroState TaskManager::pomodoroState() const {
    return pomodoroState_;
}

unsigned long TaskManager::pomodoroRemainingSec(unsigned long now) const {
    if (pomodoroState_ == PomodoroState::Stopped) return 0;
    unsigned long elapsed = now - pomodoroStartedAt_;
    if (elapsed >= pomodoroDurationMs_) return 0;
    return (pomodoroDurationMs_ - elapsed) / 1000UL;
}

void TaskManager::updatePet(unsigned long now) {
    if (pet_.lastDecayAt == 0) {
        pet_.lastDecayAt = now;
        return;
    }
    if (now - pet_.lastDecayAt >= Config::PET_DECAY_INTERVAL_MS) {
        pet_.lastDecayAt = now;
        if (pet_.hunger > 5) pet_.hunger -= 2;
        if (pet_.happiness > 5) pet_.happiness -= 1;
        saveToFlash();
    }
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
    size_t idx = random(0, NUM_ANSWERS);
    lastAnswer_ = ANSWERS[idx];
    return lastAnswer_;
}

String TaskManager::lastAnswer() const {
    return lastAnswer_;
}

String TaskManager::randomQuote() {
    size_t idx = random(0, NUM_QUOTES);
    currentQuote_ = QUOTES[idx];
    return currentQuote_;
}

String TaskManager::currentQuote() const {
    return currentQuote_;
}

void TaskManager::clearCanvas() {
    memset(canvasBuffer_, 0, sizeof(canvasBuffer_));
}

void TaskManager::setPixel(uint8_t x, uint8_t y, bool color) {
    if (x >= 128 || y >= 64) return;
    uint16_t idx = x + (y / 8) * 128;
    uint8_t bit = y % 8;
    if (color) {
        canvasBuffer_[idx] |= (1 << bit);
    } else {
        canvasBuffer_[idx] &= ~(1 << bit);
    }
}

const uint8_t* TaskManager::canvasBuffer() const {
    return canvasBuffer_;
}

void TaskManager::setGuardArmed(bool armed) {
    guardArmed_ = armed;
    if (!armed) guardAlarmTriggered_ = false;
}

bool TaskManager::isGuardArmed() const {
    return guardArmed_;
}

void TaskManager::triggerGuardAlarm() {
    if (guardArmed_) guardAlarmTriggered_ = true;
}

bool TaskManager::isGuardAlarmTriggered() const {
    return guardAlarmTriggered_;
}

void TaskManager::resetGuardAlarm() {
    guardAlarmTriggered_ = false;
}
