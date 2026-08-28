#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1305.h>
#include "AppMode.h"
#include "FaceAnimator.h"
#include "Weather.h"
#include "WiFiDiagnostics.h"

class TaskManager;

enum class Emotion { Idle, Happy, Love, Excited, Cool, Sad, Angry, Sleep, Surprised };
const char* weatherCodeText(int code);

class FaceRenderer {
public:
    FaceRenderer();
    // Returns false if the panel did not answer on I2C.
    bool begin();
    bool ready() const;
    // One frame of the boot name card. `elapsed` is milliseconds since the
    // splash began: the name drops in, the rule wipes out and the tagline
    // types itself over Config::SPLASH_INTRO_MS, and anything past that draws
    // the settled card. Call it in a loop; setup() owns the pacing.
    void showSplash(unsigned long elapsed);
    // Progress card for the parts of setup() that block. Keeps a Wi-Fi fan
    // animating so a 20-second association does not look like a hang.
    // `detail` may be nullptr.
    void showBootStatus(const char* headline, const char* detail, unsigned long now);
    // How to reach the bot, held long enough to be typed into a phone.
    // `password` is only shown in AP mode, and empty means the AP came up
    // open. The first rendered frame replaces this, so nothing clears it.
    void showNetworkCard(bool apMode, const String& ssid, const String& password, const String& ip);
    // Why the radio check ended the way it did, readable without a laptop.
    void showWiFiDiagScreen(const WiFiDiagnostics& diag);
    void render(AppMode mode, Emotion emotion, const WeatherData& weather, bool wifiOnline, unsigned long now, TaskManager* taskMgr = nullptr);
    FaceAnimator& animator() { return animator_; }
    // Changes rarely, unlike per-frame render() data, so it is kept as a
    // renderer-owned preference instead of threaded through every call.
    void setClockFormat(bool use24Hour) { use24Hour_ = use24Hour; }

private:
    Adafruit_SSD1305 display_;
    FaceAnimator animator_;
    bool ready_ = false;
    bool use24Hour_ = true;
    // Triangle wordmark for the boot card, growing from its baseline.
    void drawDeltaMark(int cx, int baseY, int size, float progress);
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
    // Centres one line at `y`, clipped to the characters the 128px panel can
    // actually fit rather than wrapping them onto the next row.
    void printCentered(const char* text, int y, uint8_t size);

    // New Screens
    void drawTasksScreen(TaskManager* taskMgr, unsigned long now);
    void drawNoticeScreen(TaskManager* taskMgr, unsigned long now);
    void drawReminderScreen(TaskManager* taskMgr, unsigned long now);
    void drawPomodoroScreen(TaskManager* taskMgr, unsigned long now);
    void drawCanvasScreen(TaskManager* taskMgr, unsigned long now);
    void drawQuotesScreen(TaskManager* taskMgr, unsigned long now);
    void drawDeskGuardScreen(TaskManager* taskMgr, unsigned long now);
    void drawPetScreen(TaskManager* taskMgr, Emotion emotion, unsigned long now);
    void drawDecisionScreen(TaskManager* taskMgr, unsigned long now);
    void drawNightScreen(unsigned long now);
    void drawRCCarFace(Emotion emotion, unsigned long now);
};