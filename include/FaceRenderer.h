#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1305.h>
#include "AppMode.h"
#include "FaceAnimator.h"
#include "Weather.h"

class TaskManager;

enum class Emotion { Idle, Happy, Love, Excited, Cool, Sad, Angry, Sleep, Surprised };
const char* weatherCodeText(int code);

class FaceRenderer {
public:
    FaceRenderer();
    // Returns false if the panel did not answer on I2C.
    bool begin();
    bool ready() const;
    void render(AppMode mode, Emotion emotion, const WeatherData& weather, bool wifiOnline, unsigned long now, TaskManager* taskMgr = nullptr);
    FaceAnimator& animator() { return animator_; }

private:
    Adafruit_SSD1305 display_;
    FaceAnimator animator_;
    bool ready_ = false;
    // --- animated face ---
    void drawAnimatedFace(const FaceFrame& f);
    void drawEye(const FaceFrame& f, int cx, int cy, bool leftEye);
    void drawBrow(const FaceFrame& f, int cx, int cy, bool leftEye);
    void drawAnimMouth(const FaceFrame& f, int cx, int baseY);
    void drawZzz(int x, int y, unsigned long now);
    void drawMusicFace(unsigned long now);
    void drawMusicNote(int x, int y, bool doubleNote);
    void drawSparkle(int x, int y, int size);
    void drawMusicEqualizer(float songTime, float beatPhase);
    void drawMusicEyes(int bounceY, int grooveX);
    void drawMusicMouth(int bounceY, int grooveX, float songTime, float beatPhase);
    void drawMusicEyebrows(int bounceY, int grooveX);
    void drawMusicParticles(float songTime);
    void drawMusicBlink(int bounceY, unsigned long now);
    void drawWeatherScreen(const WeatherData& weather, bool wifiOnline, unsigned long now);
    void drawTimeDateScreen(unsigned long now);
    void drawWeatherIcon(int weatherCode, int cx, int cy, unsigned long now);
    void drawCloud(int cx, int cy);
    // Plots an arc clockwise from startDeg (0 = 3 o'clock) for sweepDeg.
    // `dotted` skips every other step for the unfilled progress track.
    void drawArc(int cx, int cy, int rx, int ry, float startDeg, float sweepDeg, bool dotted = false);
    void drawStatus(bool wifiOnline);

    // New Screens
    void drawTasksScreen(TaskManager* taskMgr, unsigned long now);
    void drawNoticeScreen(TaskManager* taskMgr, unsigned long now);
    void drawReminderScreen(TaskManager* taskMgr, unsigned long now);
    void drawPomodoroScreen(TaskManager* taskMgr, unsigned long now);
    void drawCanvasScreen(TaskManager* taskMgr, unsigned long now);
    void drawQuotesScreen(TaskManager* taskMgr, unsigned long now);
    void drawDeskGuardScreen(TaskManager* taskMgr, unsigned long now);
    void drawPetScreen(TaskManager* taskMgr, unsigned long now);
    void drawDecisionScreen(TaskManager* taskMgr, unsigned long now);
    void drawNightScreen(unsigned long now);
    void drawRCCarFace(Emotion emotion, unsigned long now);
};