#pragma once

#include <WebServer.h>
#include "AppController.h"
#include "CommandProcessor.h"
#include "MotorController.h"

class WebController {
public:
    WebController(AppController& app, MotorController& motors, CommandProcessor& commands);
    void begin();
    void update();

private:
    AppController& app_;
    MotorController& motors_;
    CommandProcessor& commands_;
    WebServer server_{80};

    void registerRoutes();
    void sendStatus();
    void refreshWeather();
    void updateWiFi();
    void motorCommand();

    void handleTasksApi();
    void handleNoticeApi();
    void handleReminderApi();
    void handlePomodoroApi();
    void handlePetApi();
    void handleDecisionApi();
    void handleCanvasApi();
    void handleGuardApi();
    void handleQuoteApi();
    void handleDefaultModeApi();
    void handleWiFiDiagApi();
    void handleClockApi();

    // Returns true when the request may proceed; otherwise it has already been
    // answered with a 401 challenge.
    bool requireAuth();
    void sendOk();
    void sendError(int code, const char* message);

    bool restartRequested_ = false;
    unsigned long restartAt_ = 0;
    bool handleCommand(const String& command);
};
