#include "FaceRenderer.h"
#include <Wire.h>
#include <WiFi.h>
#include "Config.h"
#include "TaskManager.h"
#include "AppController.h"

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

void FaceRenderer::render(AppMode mode, Emotion emotion, const WeatherData& weather, bool wifiOnline, unsigned long now, TaskManager* taskMgr) {
    display_.clearDisplay();
    display_.setTextColor(PixelOn);

    switch (mode) {
        case AppMode::TimeDate:
            drawTimeDateScreen(now);
            break;
        case AppMode::Weather:
            drawWeatherScreen(weather, wifiOnline, now);
            break;
        case AppMode::Tasks:
            drawTasksScreen(taskMgr, now);
            break;
        case AppMode::Notice:
            drawNoticeScreen(taskMgr, now);
            break;
        case AppMode::Reminder:
            drawReminderScreen(taskMgr, now);
            break;
        case AppMode::Pomodoro:
            drawPomodoroScreen(taskMgr, now);
            break;
        case AppMode::Canvas:
            drawCanvasScreen(taskMgr, now);
            break;
        case AppMode::Quotes:
            drawQuotesScreen(taskMgr, now);
            break;
        case AppMode::DeskGuard:
            drawDeskGuardScreen(taskMgr, now);
            break;
        case AppMode::Pet:
            drawPetScreen(taskMgr, now);
            break;
        case AppMode::Decision:
            drawDecisionScreen(taskMgr, now);
            break;
        case AppMode::Night:
            drawNightScreen(now);
            break;
        case AppMode::RCCar:
            drawRCCarFace(emotion, now);
            break;
        case AppMode::Music:
        default:
            if (emotion == Emotion::Happy || emotion == Emotion::Idle) {
                drawMusicFace(now);
            } else {
                drawSimpleRobotFace(emotion, now);
            }
            break;
    }

    drawStatus(wifiOnline);
    display_.display();
}

void FaceRenderer::drawTasksScreen(TaskManager* taskMgr, unsigned long now) {
    display_.setTextSize(1);
    display_.setCursor(4, 2);
    display_.print("TASKS / TODO LIST");
    display_.drawFastHLine(4, 12, 120, PixelOn);

    if (!taskMgr || taskMgr->taskCount() == 0) {
        display_.setCursor(20, 28);
        display_.print("No active tasks");
        display_.setCursor(15, 42);
        display_.print("Add via Web App!");
        return;
    }

    int y = 16;
    for (size_t i = 0; i < taskMgr->taskCount() && i < 3; ++i) {
        const TaskItem* t = taskMgr->getTask(i);
        if (!t) continue;
        display_.setCursor(6, y);
        display_.print(t->completed ? "[X] " : "[ ] ");
        display_.print(t->text.substring(0, 14));
        y += 15;
    }
}

void FaceRenderer::drawNoticeScreen(TaskManager* taskMgr, unsigned long now) {
    display_.drawRect(2, 2, 124, 60, PixelOn);
    display_.drawRect(4, 4, 120, 56, PixelOn);

    display_.setCursor(35, 8);
    display_.print("! NOTICE !");
    display_.drawFastHLine(10, 18, 108, PixelOn);

    String text = taskMgr ? taskMgr->notice() : "Welcome to Delta-Bot!";
    if (text.length() == 0) text = "No announcements.";

    int scrollOffset = static_cast<int>((now / 150) % (text.length() * 6 + 120));
    display_.setCursor(120 - scrollOffset, 32);
    display_.print(text);
}

void FaceRenderer::drawReminderScreen(TaskManager* taskMgr, unsigned long now) {
    display_.setTextSize(1);
    display_.setCursor(28, 4);
    display_.print("REMINDER ALARM");
    display_.drawFastHLine(10, 14, 108, PixelOn);

    const bool bellRinging = (now / 200) % 2 == 0;
    int bellX = 58 + (bellRinging ? 2 : -2);
    display_.drawCircle(bellX, 26, 8, PixelOn);
    display_.fillTriangle(bellX - 8, 30, bellX + 8, 30, bellX, 22, PixelOn);

    display_.setCursor(10, 42);
    if (taskMgr && taskMgr->isReminderActive()) {
        display_.print(taskMgr->reminderTitle().substring(0, 18));
    } else {
        display_.print("No active reminder");
    }
}

void FaceRenderer::drawPomodoroScreen(TaskManager* taskMgr, unsigned long now) {
    display_.setTextSize(1);
    display_.setCursor(12, 4);
    display_.print("POMODORO TIMER");
    display_.drawFastHLine(10, 14, 108, PixelOn);

    if (!taskMgr || taskMgr->pomodoroState() == PomodoroState::Stopped) {
        display_.setCursor(25, 26);
        display_.print("Timer Stopped");
        display_.setCursor(15, 42);
        display_.print("Start via Web App");
        return;
    }

    bool isWork = taskMgr->pomodoroState() == PomodoroState::Work;
    unsigned long remSec = taskMgr->pomodoroRemainingSec(now);
    int minutes = remSec / 60;
    int seconds = remSec % 60;

    display_.setCursor(15, 22);
    display_.print(isWork ? "[WORK FOCUS]" : "[COFFEE BREAK]");

    char buf[10];
    snprintf(buf, sizeof(buf), "%02d:%02d", minutes, seconds);
    display_.setTextSize(2);
    display_.setCursor(34, 38);
    display_.print(buf);
}

void FaceRenderer::drawCanvasScreen(TaskManager* taskMgr, unsigned long now) {
    if (!taskMgr) return;
    const uint8_t* buf = taskMgr->canvasBuffer();
    for (uint8_t y = 0; y < 64; ++y) {
        for (uint8_t x = 0; x < 128; ++x) {
            uint16_t idx = x + (y / 8) * 128;
            uint8_t bit = y % 8;
            if (buf[idx] & (1 << bit)) {
                display_.drawPixel(x, y, PixelOn);
            }
        }
    }
}

void FaceRenderer::drawQuotesScreen(TaskManager* taskMgr, unsigned long now) {
    display_.drawRoundRect(4, 4, 120, 56, 4, PixelOn);
    display_.setCursor(36, 8);
    display_.print("DAILY TIP");
    display_.drawFastHLine(10, 18, 108, PixelOn);

    String quote = taskMgr ? taskMgr->currentQuote() : "Stay hungry, stay foolish.";
    display_.setCursor(8, 24);
    display_.print(quote.substring(0, 50));
}

void FaceRenderer::drawDeskGuardScreen(TaskManager* taskMgr, unsigned long now) {
    const bool flash = (now / 300) % 2 == 0;
    if (flash) {
        display_.fillRect(0, 0, 128, 64, PixelOn);
        display_.setTextColor(PixelOff);
    }

    display_.setTextSize(2);
    display_.setCursor(18, 14);
    display_.print("! BUSTED !");

    display_.setTextSize(1);
    display_.setCursor(15, 42);
    display_.print("INTRUDER DETECTED");
}

void FaceRenderer::drawPetScreen(TaskManager* taskMgr, unsigned long now) {
    const PetStats& pet = taskMgr ? taskMgr->petStats() : PetStats();
    display_.setCursor(4, 4);
    display_.print("VIRTUAL PET");
    display_.drawFastHLine(4, 14, 120, PixelOn);

    display_.setCursor(8, 22);
    display_.print("HUNGER  : ");
    display_.drawRect(68, 22, 50, 8, PixelOn);
    display_.fillRect(68, 22, (pet.hunger * 50) / 100, 8, PixelOn);

    display_.setCursor(8, 38);
    display_.print("HAPPY   : ");
    display_.drawRect(68, 38, 50, 8, PixelOn);
    display_.fillRect(68, 38, (pet.happiness * 50) / 100, 8, PixelOn);

    display_.setCursor(15, 52);
    display_.print(pet.hunger > 30 ? "I feel great! :)" : "Feed me pizza! 🍕");
}

void FaceRenderer::drawDecisionScreen(TaskManager* taskMgr, unsigned long now) {
    display_.setCursor(10, 4);
    display_.print("MAGIC 8-BALL");
    display_.drawFastHLine(10, 14, 108, PixelOn);

    String answer = taskMgr ? taskMgr->lastAnswer() : "ASK ME!";
    if (answer.length() == 0) answer = "ASK ME!";

    display_.drawCircle(64, 40, 20, PixelOn);
    display_.setTextSize(1);
    display_.setCursor(45, 36);
    display_.print(answer);
}

void FaceRenderer::drawNightScreen(unsigned long now) {
    display_.fillCircle(105, 18, 12, PixelOn);
    display_.fillCircle(100, 14, 10, PixelOff);

    const int twinkle = static_cast<int>((now / 400) % 3);
    drawSparkle(20, 12, 1 + twinkle);
    drawSparkle(55, 8, 2 - twinkle);
    drawSparkle(78, 22, 1 + twinkle);

    display_.setCursor(30, 45);
    display_.print("Good Night zZZ");
}

void FaceRenderer::drawRCCarFace(Emotion emotion, unsigned long now) {
    const int speedTrail = static_cast<int>((now / 80) % 4);
    display_.fillRoundRect(20 - speedTrail, 18, 30, 20, 6, PixelOn);
    display_.fillRoundRect(78 + speedTrail, 18, 30, 20, 6, PixelOn);

    display_.drawFastHLine(50, 28, 28, PixelOn);

    display_.setCursor(38, 48);
    display_.print("RC DRIVING");
}

void FaceRenderer::drawFace(Emotion emotion, unsigned long now) {
    drawEyes(emotion, now);
    drawMouth(emotion, now);
}

void FaceRenderer::drawEyes(Emotion emotion, unsigned long now) {
    const bool blink = (now % 5000) > 4750 && emotion != Emotion::Sleep;
    if (emotion == Emotion::Sleep || blink) {
        display_.drawLine(20, 25, 42, 25, PixelOn);
        display_.drawLine(86, 25, 108, 25, PixelOn);
        return;
    }
    display_.fillRoundRect(18, 15, 27, 21, 5, PixelOn);
    display_.fillRoundRect(84, 15, 27, 21, 5, PixelOn);
}

void FaceRenderer::drawMouth(Emotion emotion, unsigned long now) {
    if (emotion == Emotion::Sleep) return;
    display_.drawLine(52, 47, 76, 47, PixelOn);
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
    }
}

void FaceRenderer::drawMusicEyes(int bounceY, int grooveX) {
    const int ey = 22 + bounceY;
    const int leftX = 42 + grooveX;
    const int rightX = 86 + grooveX;
    display_.fillRoundRect(leftX - 16, ey - 9, 32, 18, 7, PixelOn);
    display_.fillRoundRect(rightX - 16, ey - 9, 32, 18, 7, PixelOn);
}

void FaceRenderer::drawMusicMouth(int bounceY, int grooveX, float songTime, float beatPhase) {
    const int mouthX = 64 + grooveX;
    const int mouthY = 35 + bounceY;
    display_.fillRoundRect(mouthX - 7, mouthY, 14, 6, 3, PixelOn);
}

void FaceRenderer::drawMusicEyebrows(int bounceY, int grooveX) {
    const int y = 8 + bounceY;
    display_.drawLine(32 + grooveX, y - 2, 52 + grooveX, y - 4, PixelOn);
    display_.drawLine(76 + grooveX, y - 4, 96 + grooveX, y - 2, PixelOn);
}

void FaceRenderer::drawMusicParticles(float songTime) {
    const int rightY = static_cast<int>(60 - fmod(songTime * 35.0f, 60.0f));
    drawMusicNote(110, rightY, true);
}

void FaceRenderer::drawMusicBlink(int bounceY, unsigned long now) {
    const unsigned long blinkPhase = now % 5000;
    if (blinkPhase < 180) {
        const int ey = 22 + bounceY;
        display_.fillRect(20, ey - 14, 42, 4, PixelOff);
        display_.fillRect(66, ey - 14, 42, 4, PixelOff);
    }
}

void FaceRenderer::drawWeatherScreen(const WeatherData& weather, bool wifiOnline, unsigned long now) {
    char timeText[13] = "--:--:--";
    const time_t currentTime = time(nullptr);
    struct tm timeInfo;
    if (currentTime > 100000 && localtime_r(&currentTime, &timeInfo) != nullptr) {
        strftime(timeText, sizeof(timeText), "%I:%M:%S %p", &timeInfo);
    }
    display_.setTextSize(2);
    display_.setCursor(15, 12);
    display_.print(timeText);
    display_.setTextSize(1);
    display_.setCursor(15, 38);
    display_.print("TEMP: ");
    display_.print(weather.valid ? String(weather.temperature, 0) + " C" : "-- C");
}

void FaceRenderer::drawRobotIcon(unsigned long now) {
    display_.drawRoundRect(3, 3, 24, 18, 4, PixelOn);
}

void FaceRenderer::drawWeatherIcon(int weatherCode, unsigned long now) {
    display_.fillCircle(17, 43, 6, PixelOn);
}

void FaceRenderer::drawSimpleRobotFace(Emotion emotion, unsigned long now) {
    const int bob = static_cast<int>((now / 500) % 2);
    const int eyeY = 20 + bob;
    display_.fillRoundRect(18, eyeY, 27, 21, 5, PixelOn);
    display_.fillRoundRect(84, eyeY, 27, 21, 5, PixelOn);
}

void FaceRenderer::drawTimeDateScreen(unsigned long now) {
    char timeText[13] = "--:--:--";
    const time_t currentTime = time(nullptr);
    struct tm timeInfo;
    if (currentTime > 100000 && localtime_r(&currentTime, &timeInfo) != nullptr) {
        strftime(timeText, sizeof(timeText), "%H:%M:%S", &timeInfo);
    }
    display_.setTextSize(2);
    display_.setCursor(15, 25);
    display_.print(timeText);
}