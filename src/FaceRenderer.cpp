#include "FaceRenderer.h"
#include <Wire.h>
#include <WiFi.h>
#include "Config.h"

namespace {
constexpr uint16_t PixelOn = 1;
constexpr uint16_t PixelOff = 0;
}

FaceRenderer::FaceRenderer()
    : display_(128, 64, &Wire, -1) {}

void FaceRenderer::begin() {
    Wire.begin(Config::I2C_SDA_PIN, Config::I2C_SCL_PIN);
    Wire.setClock(400000);
    display_.begin(Config::OLED_ADDRESS);
    display_.setTextColor(PixelOn);
    display_.setTextSize(1);
}

void FaceRenderer::render(AppMode mode, Emotion emotion, const WeatherData& weather, bool wifiOnline, unsigned long now) {
    display_.clearDisplay();
    display_.setTextColor(PixelOn);
    if (mode == AppMode::TimeDate) {
        drawTimeDateScreen(now);
    } else if (mode == AppMode::Weather) {
        drawWeatherScreen(weather, wifiOnline, now);
    } else {
        if (emotion == Emotion::Happy || emotion == Emotion::Idle) {
            drawMusicFace(now);
        } else {
            drawSimpleRobotFace(emotion, now);
        }
    }
    drawStatus(wifiOnline);
    display_.display();
}

void FaceRenderer::drawFace(Emotion emotion, unsigned long now) {
    drawEyes(emotion, now);
    drawMouth(emotion, now);
    if (emotion == Emotion::Love) {
        display_.setCursor(5, 5);
        display_.print("<3");
        display_.setCursor(108, 5);
        display_.print("<3");
    } else if (emotion == Emotion::Cool || emotion == Emotion::Happy) {
        display_.fillRect(17, 20, 31, 10, PixelOn);
        display_.fillRect(80, 20, 31, 10, PixelOn);
        display_.drawLine(48, 24, 80, 24, PixelOn);
    } else if (emotion == Emotion::Sleep) {
        display_.setCursor(104, 7);
        display_.print("Z Z");
    }
}

void FaceRenderer::drawEyes(Emotion emotion, unsigned long now) {
    const bool blink = (now % 5000) > 4750 && emotion != Emotion::Sleep;
    if (emotion == Emotion::Sleep || blink) {
        display_.drawLine(20, 25, 42, 25, PixelOn);
        display_.drawLine(86, 25, 108, 25, PixelOn);
        return;
    }
    if (emotion == Emotion::Surprised || emotion == Emotion::Excited) {
        display_.drawCircle(31, 25, 11, PixelOn);
        display_.drawCircle(97, 25, 11, PixelOn);
        display_.fillCircle(31, 25, 4, PixelOn);
        display_.fillCircle(97, 25, 4, PixelOn);
        return;
    }
    display_.fillRoundRect(18, 15, 27, 21, 5, PixelOn);
    display_.fillRoundRect(84, 15, 27, 21, 5, PixelOn);
    if (emotion == Emotion::Angry) {
        display_.drawLine(17, 14, 44, 20, PixelOn);
        display_.drawLine(111, 14, 85, 20, PixelOn);
    }
}

void FaceRenderer::drawMouth(Emotion emotion, unsigned long now) {
    if (emotion == Emotion::Sleep) return;
    if (emotion == Emotion::Sad) {
        display_.drawLine(56, 53, 60, 56, PixelOn);
        display_.drawLine(60, 56, 68, 56, PixelOn);
        display_.drawLine(68, 56, 72, 53, PixelOn);
    } else if (emotion == Emotion::Surprised || emotion == Emotion::Excited) {
        display_.drawRoundRect(57, 43, 14, 18, 5, PixelOn);
    } else {
        display_.drawLine(52, 43, 58, 47, PixelOn);
        display_.drawLine(58, 47, 70, 47, PixelOn);
        display_.drawLine(70, 47, 76, 43, PixelOn);
    }
    if (emotion == Emotion::Happy || emotion == Emotion::Cool) {
        const int offset = static_cast<int>((now / 100) % 4);
        for (int x = 8; x < 120; x += 18) display_.drawFastVLine(x, 50 - offset, 5 + offset, PixelOn);
    }
}

void FaceRenderer::drawStatus(bool wifiOnline) {
    display_.fillCircle(123, 4, 2, PixelOn);
    if (!wifiOnline) display_.drawCircle(123, 4, 3, PixelOn);
}

const char* weatherCodeText(int code) {
    if (code == 0) return "Clear";
    if (code <= 3) return "Cloudy";
    if (code <= 67) return "Rain";
    if (code <= 77) return "Snow";
    return "Storm";
}

void FaceRenderer::drawMusicFace(unsigned long now) {
    const float songTime = now / 1000.0f;
    const float beatPhase = fmod(songTime * (171.0f / 60.0f), 1.0f);
    const int bounceY = abs(static_cast<int>(sin(beatPhase * PI) * 5.0f));
    const int grooveX = static_cast<int>(sin(songTime * (171.0f / 120.0f) * PI) * 3.0f);

    drawMusicEqualizer(songTime, beatPhase);
    drawMusicEyebrows(bounceY, grooveX);
    drawMusicEyes(bounceY, grooveX);
    drawMusicMouth(bounceY, grooveX, songTime, beatPhase);
    drawMusicParticles(songTime);
    drawMusicBlink(bounceY, now);
}

void FaceRenderer::drawMusicNote(int x, int y, bool doubleNote) {
    if (y < 0 || y > 55) return;
    display_.fillCircle(x, y + 3, 2, PixelOn);
    display_.drawFastVLine(x + 2, y - 5, 8, PixelOn);
    if (doubleNote) {
        display_.fillCircle(x + 6, y + 1, 2, PixelOn);
        display_.drawFastVLine(x + 8, y - 7, 8, PixelOn);
        display_.fillRect(x + 2, y - 7, 7, 2, PixelOn);
    } else {
        display_.drawLine(x + 2, y - 5, x + 5, y - 3, PixelOn);
    }
}

void FaceRenderer::drawSparkle(int x, int y, int size) {
    if (size <= 0) return;
    display_.drawFastHLine(x - size, y, size * 2 + 1, PixelOn);
    display_.drawFastVLine(x, y - size, size * 2 + 1, PixelOn);
}

void FaceRenderer::drawMusicEqualizer(float songTime, float beatPhase) {
    for (int i = 0; i < 8; ++i) {
        const float wave1 = sin(songTime * 8.0f + i * 0.7f);
        const float wave2 = sin(beatPhase * PI);
        const int height = max(2, static_cast<int>(fabs(wave1) * 14.0f + wave2 * 4.0f));
        const int barX = 16 + i * 12;
        display_.fillRect(barX, 64 - height, 8, height, PixelOn);
        for (int y = 64 - height; y < 64; y += 3) display_.drawFastHLine(barX, y, 8, PixelOff);
    }
}

void FaceRenderer::drawMusicEyes(int bounceY, int grooveX) {
    const int ey = 22 + bounceY;
    const int leftX = 42 + grooveX;
    const int rightX = 86 + grooveX;
    display_.fillRoundRect(leftX - 16, ey - 9, 32, 18, 7, PixelOn);
    display_.fillRect(leftX - 18, ey, 36, 12, PixelOff);
    display_.drawFastHLine(leftX - 13, ey, 26, PixelOff);
    display_.fillRoundRect(rightX - 16, ey - 9, 32, 18, 7, PixelOn);
    display_.fillRect(rightX - 18, ey, 36, 12, PixelOff);
    display_.drawFastHLine(rightX - 13, ey, 26, PixelOff);
    display_.fillRect(58 + grooveX, ey - 6, 12, 3, PixelOn);
    for (int offset = -4; offset <= 4; offset += 4) {
        display_.drawPixel(leftX - 18 + offset, ey + 8, PixelOn);
        display_.drawPixel(rightX + 14 + offset, ey + 8, PixelOn);
    }
}

void FaceRenderer::drawMusicMouth(int bounceY, int grooveX, float songTime, float beatPhase) {
    const int mouthX = 64 + grooveX;
    const int mouthY = 35 + bounceY;
    const int mouthOpen = max(2, 4 + static_cast<int>(sin(beatPhase * PI) * 8.0f));
    const int mouthWidth = max(4, 14 + static_cast<int>(cos(songTime * 6.0f) * 4.0f));
    display_.fillRoundRect(mouthX - mouthWidth / 2, mouthY, mouthWidth, mouthOpen, 4, PixelOn);
    if (mouthOpen > 5) {
        display_.fillCircle(mouthX, mouthY + mouthOpen - 2, 2, PixelOff);
    }
}

void FaceRenderer::drawMusicEyebrows(int bounceY, int grooveX) {
    const int y = 8 + bounceY;
    display_.drawLine(32 + grooveX, y - 2, 52 + grooveX, y - 4, PixelOn);
    display_.drawLine(32 + grooveX, y - 1, 52 + grooveX, y - 3, PixelOn);
    display_.drawLine(76 + grooveX, y - 4, 96 + grooveX, y - 2, PixelOn);
    display_.drawLine(76 + grooveX, y - 3, 96 + grooveX, y - 1, PixelOn);
}

void FaceRenderer::drawMusicParticles(float songTime) {
    const int rightY = static_cast<int>(60 - fmod(songTime * 35.0f, 60.0f));
    const int leftY = static_cast<int>(60 - fmod(songTime * 35.0f + 30.0f, 60.0f));
    drawMusicNote(110 + static_cast<int>(sin(songTime * 4.0f) * 4), rightY, true);
    drawMusicNote(8 + static_cast<int>(cos(songTime * 3.5f) * 4), leftY, false);
    const int sparkleSize = static_cast<int>(fabs(sin(songTime * 6.0f)) * 3.0f);
    drawSparkle(22, 12, sparkleSize);
    drawSparkle(106, 14, sparkleSize);
}

void FaceRenderer::drawMusicBlink(int bounceY, unsigned long now) {
    const unsigned long blinkPhase = now % 5000;
    if (blinkPhase < 180) {
        const int ey = 22 + bounceY;
        const int amount = blinkPhase < 90 ? blinkPhase / 3 : (180 - blinkPhase) / 3;
        display_.fillRect(20, ey - 14, 42, amount, PixelOff);
        display_.fillRect(66, ey - 14, 42, amount, PixelOff);
        display_.fillRect(20, ey + 14 - amount, 42, amount, PixelOff);
        display_.fillRect(66, ey + 14 - amount, 42, amount, PixelOff);
    }
}

void FaceRenderer::drawWeatherScreen(const WeatherData& weather, bool wifiOnline, unsigned long now) {
    char timeText[13] = "--:--:--";
    char dateText[13] = "TIME OFFLINE";
    const time_t currentTime = time(nullptr);
    struct tm timeInfo;
    if (currentTime > 100000 && localtime_r(&currentTime, &timeInfo) != nullptr) {
        strftime(timeText, sizeof(timeText), "%I:%M:%S %p", &timeInfo);
        static const char* const days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        static const char* const months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
        snprintf(dateText, sizeof(dateText), "%s %02d %s", days[timeInfo.tm_wday], timeInfo.tm_mday, months[timeInfo.tm_mon]);
    } else {
        strcpy(timeText, "--:--:--");
        strcpy(dateText, "SYNCING TIME");
    }
    display_.drawFastHLine(3, 14, 122, PixelOn);
    drawRobotIcon(now);
    display_.setTextSize(1);
    display_.setCursor(35, 3);
    display_.print(dateText);
    display_.setTextSize(2);
    display_.setCursor(19, 16);
    display_.print(timeText);
    display_.drawFastHLine(19, 32, 90, PixelOn);
    drawWeatherIcon(weather.weatherCode, now);
    display_.setTextSize(1);
    display_.setCursor(39, 38);
    if (weather.valid) {
        display_.print(weather.temperature, 0);
        display_.print(" C  ");
        display_.print(weatherCodeText(weather.weatherCode));
    } else {
        display_.print("-- C  WEATHER OFFLINE");
    }
    display_.drawFastHLine(3, 52, 122, PixelOn);
    display_.setCursor(5, 55);
    display_.print("BAT 95%   NET ");
    display_.print(wifiOnline ? "OK" : "OFF");
}

void FaceRenderer::drawRobotIcon(unsigned long now) {
    const int bob = static_cast<int>((now / 180) % 2);
    display_.drawRoundRect(3, 3 + bob, 24, 18, 4, PixelOn);
    display_.fillCircle(10, 12 + bob, 2, PixelOn);
    display_.fillCircle(20, 12 + bob, 2, PixelOn);
    display_.drawFastHLine(10, 17 + bob, 10, PixelOn);
    display_.drawFastVLine(15, 0 + bob, 3, PixelOn);
    display_.fillCircle(15, 0 + bob, 1, PixelOn);
}

void FaceRenderer::drawWeatherIcon(int weatherCode, unsigned long now) {
    const int shift = static_cast<int>((now / 300) % 3) - 1;
    const int x = 17 + shift;
    if (weatherCode == 0) {
        display_.fillCircle(x, 43, 6, PixelOn);
        display_.drawFastHLine(x - 10, 43, 21, PixelOn);
        display_.drawFastVLine(x, 33, 21, PixelOn);
    } else {
        display_.fillCircle(x - 5, 43, 4, PixelOn);
        display_.fillCircle(x + 1, 40, 6, PixelOn);
        display_.fillCircle(x + 8, 43, 4, PixelOn);
        display_.fillRect(x - 8, 43, 20, 6, PixelOn);
        if (weatherCode >= 51) {
            display_.drawFastVLine(x - 4, 51, 5, PixelOn);
            display_.drawFastVLine(x + 3, 51, 5, PixelOn);
            display_.drawFastVLine(x + 10, 51, 5, PixelOn);
        }
    }
}

void FaceRenderer::drawSimpleRobotFace(Emotion emotion, unsigned long now) {
    const unsigned long cycle = now % 5000;
    const bool blink = cycle >= 4550 && cycle < 4700 && emotion != Emotion::Sleep;
    const int bob = emotion == Emotion::Excited ? static_cast<int>((now / 90) % 3) - 1 : static_cast<int>((now / 500) % 2);
    const int eyeY = 20 + bob;
    const int eyeWidth = emotion == Emotion::Surprised ? 24 : 27;
    const int eyeHeight = blink || emotion == Emotion::Sleep ? 4 : (emotion == Emotion::Surprised ? 25 : 21);
    const int eyeRadius = blink || emotion == Emotion::Sleep ? 2 : 7;
    const int leftX = 30 - eyeWidth / 2;
    const int rightX = 98 - eyeWidth / 2;

    if (emotion == Emotion::Angry) {
        display_.drawLine(18, 16, 42, 21, PixelOn);
        display_.drawLine(110, 21, 86, 16, PixelOn);
    } else if (emotion == Emotion::Sad) {
        display_.drawLine(18, 21, 42, 16, PixelOn);
        display_.drawLine(110, 16, 86, 21, PixelOn);
    } else if (emotion == Emotion::Happy || emotion == Emotion::Love) {
        display_.drawFastHLine(19, 16, 20, PixelOn);
        display_.drawFastHLine(89, 16, 20, PixelOn);
    }

    if (emotion == Emotion::Love) {
        display_.fillCircle(30, eyeY + 10, 10, PixelOn);
        display_.fillCircle(98, eyeY + 10, 10, PixelOn);
        display_.fillTriangle(20, eyeY + 10, 40, eyeY + 10, 30, eyeY + 22, PixelOn);
        display_.fillTriangle(88, eyeY + 10, 108, eyeY + 10, 98, eyeY + 22, PixelOn);
    } else {
        display_.fillRoundRect(leftX, eyeY, eyeWidth, eyeHeight, eyeRadius, PixelOn);
        display_.fillRoundRect(rightX, eyeY, eyeWidth, eyeHeight, eyeRadius, PixelOn);
    }

    if (emotion == Emotion::Sleep) {
        display_.setCursor(108, 12);
        display_.setTextSize(1);
        display_.print("Z");
    }

    if (emotion == Emotion::Surprised || emotion == Emotion::Excited) {
        display_.fillRoundRect(56, 47 + bob, 16, 12, 5, PixelOn);
        display_.fillCircle(64, 55 + bob, 2, PixelOff);
    } else if (emotion == Emotion::Sad) {
        display_.drawLine(55, 53 + bob, 60, 50 + bob, PixelOn);
        display_.drawLine(60, 50 + bob, 68, 50 + bob, PixelOn);
        display_.drawLine(68, 50 + bob, 73, 53 + bob, PixelOn);
    } else if (emotion == Emotion::Angry) {
        display_.drawFastHLine(56, 53 + bob, 17, PixelOn);
    } else {
        const int mouthHeight = blink ? 3 : (emotion == Emotion::Happy || emotion == Emotion::Love ? 7 : 5);
        display_.fillRoundRect(55, 48 + bob, 18, mouthHeight, 3, PixelOn);
    }
}

void FaceRenderer::drawTimeDateScreen(unsigned long now) {
    char timeText[13] = "--:--:--";
    char dateText[13] = "SYNCING TIME";
    const time_t currentTime = time(nullptr);
    struct tm timeInfo;
    if (currentTime > 100000 && localtime_r(&currentTime, &timeInfo) != nullptr) {
        strftime(timeText, sizeof(timeText), "%H:%M:%S", &timeInfo);
        strftime(dateText, sizeof(dateText), "%a %d %b", &timeInfo);
    }
    display_.setTextSize(1);
    display_.setCursor(8, 8);
    display_.print("TIME + DATE");
    display_.drawFastHLine(8, 17, 112, PixelOn);
    display_.setTextSize(2);
    display_.setCursor(15, 25);
    display_.print(timeText);
    display_.setTextSize(1);
    display_.setCursor(39, 51);
    display_.print(dateText);
}