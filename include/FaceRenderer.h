#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1305.h>
#include "Weather.h"

enum class AppMode { Music, TimeDate, Weather };
enum class Emotion { Idle, Happy, Love, Excited, Cool, Sad, Angry, Sleep, Surprised };
const char* weatherCodeText(int code);

class FaceRenderer {
public:
    FaceRenderer();
    void begin();
    void render(AppMode mode, Emotion emotion, const WeatherData& weather, bool wifiOnline, unsigned long now);

private:
    Adafruit_SSD1305 display_;
    void drawFace(Emotion emotion, unsigned long now);
    void drawEyes(Emotion emotion, unsigned long now);
    void drawMouth(Emotion emotion, unsigned long now);
    void drawMusicFace(unsigned long now);
    void drawSimpleRobotFace(Emotion emotion, unsigned long now);
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
    void drawRobotIcon(unsigned long now);
    void drawWeatherIcon(int weatherCode, unsigned long now);
    void drawStatus(bool wifiOnline);
};