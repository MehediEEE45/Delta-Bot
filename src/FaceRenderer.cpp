#include "FaceRenderer.h"
#include <Wire.h>
#include <WiFi.h>
#include "Config.h"
#include "TaskManager.h"
#include <math.h>
#include <string.h>

namespace {
constexpr uint16_t PixelOn = 1;
constexpr uint16_t PixelOff = 0;
}

FaceRenderer::FaceRenderer()
    : display_(128, 64, &Wire, -1) {}

bool FaceRenderer::begin() {
    Wire.begin(Config::I2C_SDA_PIN, Config::I2C_SCL_PIN);
    Wire.setClock(Config::I2C_CLOCK_HZ);
    ready_ = display_.begin(Config::OLED_ADDRESS);
    if (!ready_) {
        Serial.print("OLED init FAILED at address 0x");
        Serial.println(Config::OLED_ADDRESS, HEX);
        Serial.println("Check SDA/SCL wiring, pull-ups, and the panel controller (SSD1305 vs SSD1306).");
        return false;
    }
    display_.setTextColor(PixelOn);
    display_.setTextSize(1);
    return true;
}

bool FaceRenderer::ready() const { return ready_; }

void FaceRenderer::render(AppMode mode, Emotion emotion, const WeatherData& weather, bool wifiOnline, unsigned long now, TaskManager* taskMgr) {
    if (!ready_) return;

    display_.clearDisplay();
    display_.setTextColor(PixelOn);
    // Screens are free to change size mid-draw; reset it so the next frame
    // never inherits a size-2 font from the screen that ran before it.
    display_.setTextSize(1);

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
        case AppMode::Face:
            animator_.update(emotion, now);
            drawAnimatedFace(animator_.frame());
            if (animator_.frame().showZzz) drawZzz(96, 24, now);
            break;
        case AppMode::RCCar:
            drawRCCarFace(emotion, now);
            break;
        case AppMode::Music:
        default:
            if (emotion == Emotion::Happy || emotion == Emotion::Idle) {
                drawMusicFace(now);
            } else {
                animator_.update(emotion, now);
                drawAnimatedFace(animator_.frame());
            }
            break;
    }

    drawStatus(wifiOnline);
    display_.display();
}

void FaceRenderer::drawTasksScreen(TaskManager* taskMgr, unsigned long) {
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

    // hasActiveNotice() is what makes the broadcast duration mean anything;
    // without it an expired notice scrolled forever.
    String text;
    if (taskMgr && taskMgr->hasActiveNotice(now)) {
        text = taskMgr->notice();
    } else {
        text = "No announcements.";
    }

    const int span = static_cast<int>(text.length()) * 6 + 120;
    const int scrollOffset = static_cast<int>((now / 150) % static_cast<unsigned long>(span));
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
        if (taskMgr->isReminderDue()) {
            display_.setCursor(10, 54);
            display_.print("DUE - tap to clear");
        }
    } else {
        display_.print("No active reminder");
    }
}

void FaceRenderer::drawPomodoroScreen(TaskManager* taskMgr, unsigned long now) {
    display_.setTextSize(1);

    if (!taskMgr || taskMgr->pomodoroState() == PomodoroState::Stopped) {
        display_.setCursor(12, 4);
        display_.print("POMODORO TIMER");
        display_.drawFastHLine(10, 14, 108, PixelOn);
        display_.setCursor(25, 26);
        display_.print("Timer Stopped");
        display_.setCursor(15, 42);
        display_.print("Start via Web App");
        return;
    }

    const PomodoroState state = taskMgr->pomodoroState();
    const unsigned long remSec = taskMgr->pomodoroRemainingSec(now);
    const int minutes = static_cast<int>(remSec / 60);
    const int seconds = static_cast<int>(remSec % 60);

    const char* label = state == PomodoroState::Paused ? "PAUSED"
                      : (state == PomodoroState::Work ? "WORK FOCUS" : "COFFEE BREAK");
    display_.setCursor(64 - static_cast<int>(strlen(label)) * 3, 2);
    display_.print(label);

    // Progress ring: dotted track for the full interval, solid 2px arc for the
    // elapsed portion, sweeping clockwise from 12 o'clock.
    const int cx = 64;
    const int cy = 38;
    const int r = 21;
    drawArc(cx, cy, r, r, 0.0f, 360.0f, true);

    const float sweep = 360.0f * taskMgr->pomodoroProgress(now);
    if (sweep > 0.0f) {
        drawArc(cx, cy, r, r, -90.0f, sweep);
        drawArc(cx, cy, r - 1, r - 1, -90.0f, sweep);
    }

    // A paused timer blinks its head so a stalled ring is not mistaken for a
    // running one.
    if (state != PomodoroState::Paused || (now / 400) % 2 == 0) {
        const float headA = (-90.0f + sweep) * DEG_TO_RAD;
        display_.fillCircle(cx + static_cast<int>(lroundf(cosf(headA) * r)),
                            cy + static_cast<int>(lroundf(sinf(headA) * r)), 2, PixelOn);
    }

    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", minutes, seconds);
    display_.setTextSize(1);
    display_.setCursor(cx - 15, cy - 4);
    display_.print(buf);
}

void FaceRenderer::drawCanvasScreen(TaskManager* taskMgr, unsigned long now) {
    (void)now;
    if (!taskMgr) return;
    // One blit instead of 8192 bounds-checked drawPixel calls per frame.
    display_.drawBitmap(0, 0, taskMgr->canvasBuffer(),
                        Config::CANVAS_WIDTH, Config::CANVAS_HEIGHT, PixelOn);
}

void FaceRenderer::drawQuotesScreen(TaskManager* taskMgr, unsigned long) {
    display_.drawRoundRect(4, 4, 120, 56, 4, PixelOn);
    display_.setCursor(36, 8);
    display_.print("DAILY TIP");
    display_.drawFastHLine(10, 18, 108, PixelOn);

    const String quote = taskMgr ? taskMgr->currentQuote() : "Stay hungry, stay foolish.";
    // 18 chars fit between the rounded border at size 1; wrap onto three lines.
    constexpr size_t lineChars = 18;
    for (size_t line = 0; line < 3; ++line) {
        const size_t start = line * lineChars;
        if (start >= quote.length()) break;
        display_.setCursor(10, 24 + static_cast<int>(line) * 10);
        display_.print(quote.substring(start, start + lineChars));
    }
}

void FaceRenderer::drawDeskGuardScreen(TaskManager*, unsigned long now) {
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
    (void)now;
    if (!taskMgr) return;
    const PetStats& pet = taskMgr->petStats();
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
    display_.print(pet.hunger > 30 ? "I feel great! :)" : "Feed me! I am hungry");
}

void FaceRenderer::drawDecisionScreen(TaskManager* taskMgr, unsigned long) {
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
    animator_.update(emotion, now);
    drawAnimatedFace(animator_.frame());

    // Speed streaks down both edges, scrolling to suggest travel.
    const int phase = static_cast<int>((now / 60) % 12);
    for (int i = 0; i < 3; ++i) {
        const int y = 14 + i * 16 + phase / 3;
        const int len = 6 + ((phase + i * 4) % 6);
        display_.drawFastHLine(0, y, len, PixelOn);
        display_.drawFastHLine(128 - len, y + 6, len, PixelOn);
    }
}




// ---------------------------------------------------------------------------
// Animated face. Geometry arrives fully resolved in a FaceFrame; nothing here
// keeps animation state of its own.
// ---------------------------------------------------------------------------

void FaceRenderer::drawEye(const FaceFrame& f, int cx, int cy, bool leftEye) {
    const int w = static_cast<int>(lroundf(f.p.eyeW));
    const int full = static_cast<int>(lroundf(f.p.eyeH));
    // Blink squashes toward the lower lid rather than toward the eye centre,
    // which is how a real lid closes.
    const int h = max(2, static_cast<int>(lroundf(f.p.eyeH * f.openness)));
    const int bottom = cy + full / 2;
    const int top = bottom - h;
    const int r = min(static_cast<int>(lroundf(f.p.eyeRadius)), min(w, h) / 2);

    switch (f.shape) {
        case EyeShape::HappyArc: {
            // A wide, shallow upward arc: the classic happy squint. An elliptical
            // arc keeps it from becoming a tall arch that crowds the brow.
            const int rx = w / 2;
            const int ry = max(4, static_cast<int>(h * 0.62f));
            for (int k = 0; k < 3; ++k) {
                drawArc(cx, bottom - 1, rx - k, ry - k, 180.0f, 180.0f);
            }
            break;
        }
        case EyeShape::Heart: {
            const int hr = max(3, w / 4);
            display_.fillCircle(cx - hr + 1, top + hr, hr, PixelOn);
            display_.fillCircle(cx + hr - 1, top + hr, hr, PixelOn);
            display_.fillTriangle(cx - 2 * hr + 1, top + hr + 1,
                                  cx + 2 * hr - 1, top + hr + 1,
                                  cx, bottom, PixelOn);
            break;
        }
        case EyeShape::Star: {
            const int a = w / 2;
            const int b = h / 2;
            display_.fillTriangle(cx, cy - b, cx - a / 2, cy, cx + a / 2, cy, PixelOn);
            display_.fillTriangle(cx, cy + b, cx - a / 2, cy, cx + a / 2, cy, PixelOn);
            display_.fillTriangle(cx - a, cy, cx, cy - b / 2, cx, cy + b / 2, PixelOn);
            display_.fillTriangle(cx + a, cy, cx, cy - b / 2, cx, cy + b / 2, PixelOn);
            break;
        }
        case EyeShape::Hollow: {
            display_.fillRoundRect(cx - w / 2, top, w, h, r, PixelOn);
            const int iw = max(2, w - 8);
            const int ih = max(2, h - 8);
            display_.fillRoundRect(cx - iw / 2, top + (h - ih) / 2, iw, ih,
                                   max(1, r - 4), PixelOff);
            if (f.showPupil && f.openness > 0.6f) {
                display_.fillCircle(cx + static_cast<int>(f.gazeX * 3),
                                    cy + static_cast<int>(f.gazeY * 2), 2, PixelOn);
            }
            break;
        }
        case EyeShape::Line:
            display_.fillRect(cx - w / 2, cy, w, max(2, h), PixelOn);
            break;
        case EyeShape::Shades:
            break; // drawn once for both eyes in drawAnimatedFace
        case EyeShape::Block:
        default: {
            display_.fillRoundRect(cx - w / 2, top, w, h, r, PixelOn);
            // The pupil is a hole punched out of the lit eye: on a 1-bit panel
            // that reads as a highlight and gives the face a direction of gaze.
            if (f.showPupil && f.openness > 0.55f) {
                const int pw = max(4, w / 3);
                const int ph = max(4, h / 3);
                const int px = cx + static_cast<int>(lroundf(f.gazeX * (w / 2.0f - pw / 2.0f - 3)));
                const int py = cy + static_cast<int>(lroundf(f.gazeY * (h / 2.0f - ph / 2.0f - 3)));
                display_.fillRoundRect(px - pw / 2, py - ph / 2, pw, ph, 2, PixelOff);
            }
            break;
        }
    }
    (void)leftEye;
}

void FaceRenderer::drawBrow(const FaceFrame& f, int cx, int cy, bool leftEye) {
    if (f.p.browLift <= 0.5f) return;
    const int w = static_cast<int>(lroundf(f.p.eyeW));
    const int tilt = static_cast<int>(lroundf(f.p.browAngle));
    const int eyeTop = cy - static_cast<int>(lroundf(f.p.eyeH / 2.0f));
    // Keep the whole stroke on the panel and clear of the eye it sits above,
    // whatever the expression asks for.
    int baseY = eyeTop - static_cast<int>(lroundf(f.p.browLift));
    baseY = max(baseY, 1 + abs(tilt));
    baseY = min(baseY, eyeTop - 2 - abs(tilt));
    if (baseY < 1) return;

    // browAngle raises the inner end for sad, drops it for angry.
    const int innerX = leftEye ? cx + w / 2 : cx - w / 2;
    const int outerX = leftEye ? cx - w / 2 : cx + w / 2;
    const int innerY = baseY - tilt;
    const int outerY = baseY + tilt;

    display_.drawLine(innerX, innerY, outerX, outerY, PixelOn);
    display_.drawLine(innerX, innerY + 1, outerX, outerY + 1, PixelOn);
}

void FaceRenderer::drawAnimMouth(const FaceFrame& f, int cx, int baseY) {
    const int w = static_cast<int>(lroundf(f.p.mouthW));
    if (w < 4) return;

    if (f.p.mouthOpen > 1.0f) {
        const int oh = static_cast<int>(lroundf(f.p.mouthOpen));
        display_.fillRoundRect(cx - w / 2, baseY - oh / 2, w, oh, min(w, oh) / 2, PixelOn);
        return;
    }

    // Parabola through the mouth width: positive curve drops the centre below
    // the corners, which reads as a smile.
    const float curve = f.p.mouthCurve;
    for (int x = -w / 2; x <= w / 2; ++x) {
        const float n = (2.0f * x) / w;
        const int y = baseY + static_cast<int>(lroundf(curve * (1.0f - n * n)));
        display_.drawPixel(cx + x, y, PixelOn);
        display_.drawPixel(cx + x, y + 1, PixelOn);
    }
}

void FaceRenderer::drawZzz(int x, int y, unsigned long now) {
    for (int i = 0; i < 3; ++i) {
        const unsigned long phase = (now / 18 + i * 500) % 1500;
        const int rise = static_cast<int>(phase / 100);
        const int zx = x + i * 3 + rise / 3;
        const int zy = y - rise;
        if (zy < 2) continue;
        const int sz = 2 + i;
        display_.drawFastHLine(zx, zy, sz, PixelOn);
        display_.drawLine(zx + sz - 1, zy, zx, zy + sz, PixelOn);
        display_.drawFastHLine(zx, zy + sz, sz, PixelOn);
    }
}

void FaceRenderer::drawAnimatedFace(const FaceFrame& f) {
    const int ox = static_cast<int>(lroundf(f.offsetX));
    const int oy = static_cast<int>(lroundf(f.offsetY));
    const int cy = static_cast<int>(lroundf(f.p.eyeY)) + oy;
    const int half = static_cast<int>(lroundf(f.p.eyeGap / 2.0f));
    const int leftX = 64 - half + ox;
    const int rightX = 64 + half + ox;

    if (f.shape == EyeShape::Shades) {
        // One visor spanning both eyes, with a bridge across the middle.
        const int h = max(3, static_cast<int>(lroundf(f.p.eyeH * f.openness)));
        const int top = cy + static_cast<int>(lroundf(f.p.eyeH / 2.0f)) - h;
        const int w = static_cast<int>(lroundf(f.p.eyeW));
        display_.fillRoundRect(leftX - w / 2, top, w, h, 3, PixelOn);
        display_.fillRoundRect(rightX - w / 2, top, w, h, 3, PixelOn);
        display_.fillRect(leftX + w / 2, top + h / 3, rightX - leftX - w, 3, PixelOn);
    } else {
        drawEye(f, leftX, cy, true);
        drawEye(f, rightX, cy, false);
        drawBrow(f, leftX, cy, true);
        drawBrow(f, rightX, cy, false);
    }

    drawAnimMouth(f, 64 + ox, static_cast<int>(lroundf(f.p.mouthY)) + oy);
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

void FaceRenderer::drawMusicNote(int x, int y, bool) {
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

void FaceRenderer::drawMusicMouth(int bounceY, int grooveX, float, float) {
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
    (void)wifiOnline; // drawStatus() already reports the link

    char timeText[8] = "--:--";
    const time_t currentTime = time(nullptr);
    struct tm timeInfo;
    if (currentTime > 100000 && localtime_r(&currentTime, &timeInfo) != nullptr) {
        strftime(timeText, sizeof(timeText), "%H:%M", &timeInfo);
    }
    display_.setTextSize(1);
    display_.setCursor(4, 3);
    display_.print(timeText);
    display_.drawFastHLine(0, 13, 128, PixelOn);

    drawWeatherIcon(weather.valid ? weather.weatherCode : -1, 26, 36, now);

    display_.setTextSize(2);
    display_.setCursor(56, 24);
    if (weather.valid) {
        display_.print(static_cast<int>(lroundf(weather.temperature)));
        display_.print("C");
    } else {
        display_.print("--");
    }

    display_.setTextSize(1);
    display_.setCursor(56, 46);
    display_.print(weather.valid ? weatherCodeText(weather.weatherCode) : "No data");
}

void FaceRenderer::drawArc(int cx, int cy, int rx, int ry, float startDeg, float sweepDeg, bool dotted) {
    if (rx <= 0 || ry <= 0 || sweepDeg == 0.0f) return;
    // Step small enough that neighbouring points stay under ~1.2px apart at the
    // largest radius we use, so a solid arc has no gaps; the dotted progress
    // track skips every other step.
    const int steps = max(2, static_cast<int>(fabs(sweepDeg) / 3.0f) * max(1, max(rx, ry) / 22));
    for (int i = 0; i <= steps; ++i) {
        if (dotted && (i % 2)) continue;
        const float a = (startDeg + sweepDeg * i / steps) * DEG_TO_RAD;
        display_.drawPixel(cx + static_cast<int>(lroundf(cosf(a) * rx)),
                           cy + static_cast<int>(lroundf(sinf(a) * ry)), PixelOn);
    }
}

void FaceRenderer::drawCloud(int cx, int cy) {
    display_.fillCircle(cx - 6, cy + 1, 5, PixelOn);
    display_.fillCircle(cx + 5, cy + 1, 5, PixelOn);
    display_.fillCircle(cx - 1, cy - 3, 7, PixelOn);
    display_.fillRect(cx - 6, cy + 1, 12, 5, PixelOn);
}

void FaceRenderer::drawWeatherIcon(int weatherCode, int cx, int cy, unsigned long now) {
    // Codes follow weatherCodeText(): 0 clear, <=3 cloud, <=67 rain, <=77 snow.
    if (weatherCode < 0) {
        display_.drawCircle(cx, cy, 11, PixelOn);
        display_.setTextSize(2);
        display_.setCursor(cx - 5, cy - 7);
        display_.print("?");
        display_.setTextSize(1);
        return;
    }

    if (weatherCode == 0) { // clear: sun with rotating rays
        display_.fillCircle(cx, cy, 7, PixelOn);
        const float spin = (now / 90.0f) * DEG_TO_RAD;
        for (int i = 0; i < 8; ++i) {
            const float a = spin + i * (PI / 4.0f);
            const int x0 = cx + static_cast<int>(lroundf(cosf(a) * 10));
            const int y0 = cy + static_cast<int>(lroundf(sinf(a) * 10));
            const int x1 = cx + static_cast<int>(lroundf(cosf(a) * 14));
            const int y1 = cy + static_cast<int>(lroundf(sinf(a) * 14));
            display_.drawLine(x0, y0, x1, y1, PixelOn);
        }
        return;
    }

    drawCloud(cx, cy - 4);

    if (weatherCode <= 3) return; // cloud only

    const int drift = static_cast<int>((now / 160) % 4);

    if (weatherCode <= 67) { // rain: slanted streaks falling on a loop
        for (int i = 0; i < 3; ++i) {
            const int x = cx - 7 + i * 7;
            const int y = cy + 6 + ((drift + i * 2) % 4);
            display_.drawLine(x, y, x - 2, y + 4, PixelOn);
        }
        return;
    }

    if (weatherCode <= 77) { // snow: drifting sparkles
        for (int i = 0; i < 3; ++i) {
            drawSparkle(cx - 7 + i * 7, cy + 8 + ((drift + i) % 3), 2);
        }
        return;
    }

    // storm: lightning bolt under the cloud
    display_.fillTriangle(cx + 1, cy + 5, cx - 5, cy + 13, cx, cy + 12, PixelOn);
    display_.fillTriangle(cx - 1, cy + 16, cx + 5, cy + 8, cx, cy + 9, PixelOn);
}


void FaceRenderer::drawTimeDateScreen(unsigned long) {
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