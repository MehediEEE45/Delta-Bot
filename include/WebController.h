#pragma once

#include <WebServer.h>
#include "AppController.h"
#include "MotorController.h"

class WebController {
public:
    WebController(AppController& app, MotorController& motors);
    void begin();
    void update();

private:
    AppController& app_;
    MotorController& motors_;
    WebServer server_{80};
    void registerRoutes();
    void sendStatus();
    void refreshWeather();
    void updateSettings();
    void updateTime();
    void updateWiFi();
    void motorCommand();

    // New API Handlers
    void handleTasksApi();
    void handleNoticeApi();
    void handleReminderApi();
    void handlePomodoroApi();
    void handlePetApi();
    void handleDecisionApi();
    void handleCanvasApi();
    void handleGuardApi();

    bool restartRequested_ = false;
    bool handleCommand(const String& command);
};
