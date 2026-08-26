#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <time.h>
#include "Config.h"

struct TaskItem {
    String text;
    bool completed = false;
};

enum class PomodoroState { Stopped, Work, Break, Paused };

struct PetStats {
    uint8_t hunger = 80;    // 0 = Starving, 100 = Full
    uint8_t happiness = 90; // 0 = Sad, 100 = Ecstatic
    unsigned long lastDecayAt = 0;
};

class TaskManager {
public:
    static constexpr size_t MAX_TASKS = 5;

    TaskManager();
    void begin();

    // Tasks API
    bool addTask(const String& text);
    bool toggleTask(size_t index);
    bool deleteTask(size_t index);
    void clearTasks();
    size_t taskCount() const;
    const TaskItem* getTask(size_t index) const;

    // Notice API
    void setNotice(const String& notice, unsigned long durationMs = 30000);
    String notice() const;
    bool hasActiveNotice(unsigned long now) const;

    // Reminder API
    void setReminder(const String& title, unsigned long targetTimeSec);
    String reminderTitle() const;
    unsigned long reminderTargetTime() const;
    bool isReminderActive() const;
    bool isReminderDue() const;
    // True exactly once, on the first call after the reminder comes due.
    bool consumeReminderTrigger();
    void clearReminder();

    // Pomodoro API
    void startPomodoro(unsigned long now);
    void pausePomodoro(unsigned long now);
    void resetPomodoro();
    void updatePomodoro(unsigned long now);
    PomodoroState pomodoroState() const;
    const char* pomodoroStateName() const;
    unsigned long pomodoroRemainingSec(unsigned long now) const;
    // 0.0 at the start of the current interval, 1.0 at its end. Drives the ring.
    float pomodoroProgress(unsigned long now) const;

    // Virtual Pet API
    void updatePet(unsigned long now);
    void feedPet();
    void petPet();
    const PetStats& petStats() const;

    // Decision 8-Ball API
    String askDecision(const String& question);
    String lastAnswer() const;

    // Quotes API
    String randomQuote();
    String currentQuote() const;

    // Canvas API
    void clearCanvas();
    void setPixel(uint8_t x, uint8_t y, bool color);
    const uint8_t* canvasBuffer() const;

    // Desk Guard API
    void setGuardArmed(bool armed);
    bool isGuardArmed() const;
    void triggerGuardAlarm();
    bool isGuardAlarmTriggered() const;
    // True exactly once per alarm, so the caller can fire a warning wiggle
    // without TaskManager needing to know about motors.
    bool consumeGuardAlarmPulse();
    void resetGuardAlarm();

    void saveToFlash();
    void loadFromFlash();
    // Commits pending background changes at most once per FLASH_THROTTLE_MS.
    void maybePersist(unsigned long now);

private:
    void markDirty();

    Preferences prefs_;

    TaskItem tasks_[MAX_TASKS];
    size_t taskCount_ = 0;

    String notice_;
    unsigned long noticeExpiresAt_ = 0;

    String reminderTitle_;
    unsigned long reminderTargetTime_ = 0;
    bool reminderActive_ = false;
    bool reminderFired_ = false;

    PomodoroState pomodoroState_ = PomodoroState::Stopped;
    unsigned long pomodoroStartedAt_ = 0;
    unsigned long pomodoroDurationMs_ = 0;
    unsigned long pomodoroPausedRemainingMs_ = 0;

    PetStats pet_;

    String lastAnswer_;
    String currentQuote_;

    uint8_t canvasBuffer_[Config::CANVAS_BUFFER_BYTES];

    bool guardArmed_ = false;
    bool guardAlarmTriggered_ = false;
    bool guardPulseFired_ = false;

    bool dirty_ = false;
    unsigned long lastSaveAt_ = 0;
};
