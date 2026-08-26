#include "FaceAnimator.h"
#include "FaceRenderer.h" // for Emotion
#include "Config.h"
#include <math.h>

namespace {

inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Ease in/out. Keeps morphs and saccades from starting and stopping abruptly,
// which is most of what separates "animated" from "teleporting".
inline float easeInOut(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

FaceParams mix(const FaceParams& a, const FaceParams& b, float t) {
    FaceParams o;
    o.eyeW       = lerp(a.eyeW,       b.eyeW,       t);
    o.eyeH       = lerp(a.eyeH,       b.eyeH,       t);
    o.eyeRadius  = lerp(a.eyeRadius,  b.eyeRadius,  t);
    o.eyeGap     = lerp(a.eyeGap,     b.eyeGap,     t);
    o.eyeY       = lerp(a.eyeY,       b.eyeY,       t);
    o.browLift   = lerp(a.browLift,   b.browLift,   t);
    o.browAngle  = lerp(a.browAngle,  b.browAngle,  t);
    o.mouthW     = lerp(a.mouthW,     b.mouthW,     t);
    o.mouthCurve = lerp(a.mouthCurve, b.mouthCurve, t);
    o.mouthOpen  = lerp(a.mouthOpen,  b.mouthOpen,  t);
    o.mouthY     = lerp(a.mouthY,     b.mouthY,     t);
    return o;
}

// random() is the hardware RNG on ESP32 and is not available in a native test
// build, so route it through one place.
inline long pick(long lo, long hi) {
#ifdef ARDUINO
    return random(lo, hi);
#else
    return lo + (rand() % (hi - lo));
#endif
}

} // namespace

FaceParams FaceAnimator::paramsFor(Emotion emotion) {
    FaceParams p; // defaults are the Idle face
    switch (emotion) {
        case Emotion::Happy:
            p.eyeH = 16.0f; p.eyeRadius = 7.0f;
            p.browLift = 5.0f; p.browAngle = 2.0f;
            p.mouthW = 28.0f; p.mouthCurve = 6.0f;
            break;
        case Emotion::Love:
            p.eyeW = 24.0f; p.eyeH = 22.0f;
            p.mouthW = 20.0f; p.mouthCurve = 5.0f;
            break;
        case Emotion::Excited:
            p.eyeW = 26.0f; p.eyeH = 24.0f;
            p.eyeY = 26.0f; p.browLift = 4.0f; p.browAngle = 3.0f;
            p.mouthW = 18.0f; p.mouthOpen = 11.0f;
            break;
        case Emotion::Cool:
            p.eyeH = 14.0f; p.eyeRadius = 3.0f;
            p.mouthW = 24.0f; p.mouthCurve = 3.0f; p.mouthY = 50.0f;
            break;
        case Emotion::Sad:
            p.eyeH = 17.0f; p.eyeY = 29.0f;
            p.browLift = 5.0f; p.browAngle = 5.0f;   // inner ends up
            p.mouthW = 22.0f; p.mouthCurve = -6.0f; p.mouthY = 52.0f;
            break;
        case Emotion::Angry:
            p.eyeH = 16.0f; p.eyeRadius = 3.0f;
            p.browLift = 5.0f; p.browAngle = -5.0f;  // inner ends down
            p.mouthW = 20.0f; p.mouthCurve = -5.0f;
            break;
        case Emotion::Surprised:
            p.eyeW = 24.0f; p.eyeH = 26.0f; p.eyeRadius = 11.0f;
            p.eyeY = 27.0f; p.browLift = 4.0f; p.browAngle = 2.0f;
            p.mouthW = 14.0f; p.mouthOpen = 13.0f; p.mouthY = 49.0f;
            break;
        case Emotion::Sleep:
            p.eyeH = 3.0f; p.eyeRadius = 1.0f; p.eyeY = 30.0f;
            p.mouthW = 0.0f;
            break;
        default: // Idle
            p.mouthW = 20.0f; p.mouthCurve = 1.5f;
            break;
    }
    return p;
}

EyeShape FaceAnimator::shapeFor(Emotion emotion) {
    switch (emotion) {
        case Emotion::Happy:     return EyeShape::HappyArc;
        case Emotion::Love:      return EyeShape::Heart;
        case Emotion::Excited:   return EyeShape::Star;
        case Emotion::Cool:      return EyeShape::Shades;
        case Emotion::Surprised: return EyeShape::Hollow;
        case Emotion::Sleep:     return EyeShape::Line;
        default:                 return EyeShape::Block;
    }
}

void FaceAnimator::begin(unsigned long now) {
    current_ = static_cast<Emotion>(0); // Idle
    from_ = to_ = paramsFor(current_);
    frame_.p = to_;
    frame_.shape = shapeFor(current_);
    morphStartedAt_ = now;
    nextBlinkAt_ = now + Config::BLINK_MIN_GAP_MS;
    nextSaccadeAt_ = now + Config::SACCADE_MIN_GAP_MS;
    saccadeStartedAt_ = now;
    started_ = true;
}

void FaceAnimator::lookAt(float x, float y) {
    lookHeld_ = true;
    gazeToX_ = x < -1.0f ? -1.0f : (x > 1.0f ? 1.0f : x);
    gazeToY_ = y < -1.0f ? -1.0f : (y > 1.0f ? 1.0f : y);
}

void FaceAnimator::releaseLook() {
    lookHeld_ = false;
}

void FaceAnimator::stepBlink(unsigned long now) {
    // Sleep has its lids shut already; blinking on top of it just jitters.
    if (frame_.shape == EyeShape::Line) {
        frame_.openness = 1.0f;
        blinking_ = false;
        nextBlinkAt_ = now + Config::BLINK_MIN_GAP_MS;
        return;
    }

    if (!blinking_ && static_cast<long>(now - nextBlinkAt_) >= 0) {
        blinking_ = true;
        blinkStartedAt_ = now;
        queueSecondBlink_ = pick(0, 100) < Config::BLINK_DOUBLE_PERCENT;
    }

    if (!blinking_) {
        frame_.openness = 1.0f;
        return;
    }

    const unsigned long elapsed = now - blinkStartedAt_;
    if (elapsed >= Config::BLINK_MS) {
        blinking_ = false;
        frame_.openness = 1.0f;
        if (queueSecondBlink_) {
            queueSecondBlink_ = false;
            nextBlinkAt_ = now + 90; // the quick second beat of a double blink
        } else {
            nextBlinkAt_ = now + static_cast<unsigned long>(
                pick(Config::BLINK_MIN_GAP_MS, Config::BLINK_MAX_GAP_MS));
        }
        return;
    }

    // Triangle across the window, eased: open -> shut -> open. The renderer
    // anchors the bottom lid, so the eye squashes downward rather than
    // shrinking toward its centre.
    const float t = static_cast<float>(elapsed) / Config::BLINK_MS;
    const float shut = t < 0.5f ? (t * 2.0f) : ((1.0f - t) * 2.0f);
    frame_.openness = 1.0f - easeInOut(shut);
}

void FaceAnimator::stepGaze(unsigned long now) {
    if (lookHeld_) {
        // Deliberate look: ease toward it and suspend wandering.
        const float k = 0.25f;
        frame_.gazeX += (gazeToX_ - frame_.gazeX) * k;
        frame_.gazeY += (gazeToY_ - frame_.gazeY) * k;
        nextSaccadeAt_ = now + Config::SACCADE_MIN_GAP_MS;
        return;
    }

    if (static_cast<long>(now - nextSaccadeAt_) >= 0) {
        gazeFromX_ = frame_.gazeX;
        gazeFromY_ = frame_.gazeY;
        gazeToX_ = pick(-70, 71) / 100.0f;
        gazeToY_ = pick(-45, 46) / 100.0f;
        saccadeStartedAt_ = now;
        nextSaccadeAt_ = now + static_cast<unsigned long>(
            pick(Config::SACCADE_MIN_GAP_MS, Config::SACCADE_MAX_GAP_MS));
    }

    // Saccades are fast and then hold: the eye snaps, it does not glide.
    const float t = clamp01(static_cast<float>(now - saccadeStartedAt_) /
                            Config::SACCADE_TRAVEL_MS);
    const float e = easeInOut(t);
    frame_.gazeX = lerp(gazeFromX_, gazeToX_, e);
    frame_.gazeY = lerp(gazeFromY_, gazeToY_, e);
}

void FaceAnimator::update(Emotion emotion, unsigned long now) {
    if (!started_) begin(now);

    if (emotion != current_) {
        // Freeze the currently displayed geometry as the morph's start point,
        // so interrupting a morph mid-way does not snap.
        from_ = frame_.p;
        to_ = paramsFor(emotion);
        current_ = emotion;
        frame_.shape = shapeFor(emotion);
        morphStartedAt_ = now;
    }

    const float mt = clamp01(static_cast<float>(now - morphStartedAt_) / Config::FACE_MORPH_MS);
    frame_.p = mix(from_, to_, easeInOut(mt));

    stepBlink(now);
    stepGaze(now);

    const float t = static_cast<float>(now);
    // Two idle sines on periods that do not divide evenly, so the drift never
    // visibly repeats, plus a much slower drift that walks the whole face
    // around the panel to spread OLED wear.
    const float idleX = sinf(t / Config::IDLE_DRIFT_X_MS * TWO_PI) * Config::IDLE_DRIFT_PX;
    const float idleY = sinf(t / Config::IDLE_DRIFT_Y_MS * TWO_PI) * Config::IDLE_DRIFT_PX;
    const float burnX = sinf(t / Config::BURNIN_X_MS * TWO_PI) * Config::BURNIN_PX;
    const float burnY = cosf(t / Config::BURNIN_Y_MS * TWO_PI) * Config::BURNIN_PX;

    frame_.offsetX = idleX + burnX;
    frame_.offsetY = idleY + burnY;
    frame_.showPupil = frame_.shape == EyeShape::Block || frame_.shape == EyeShape::Hollow;
    frame_.showZzz = frame_.shape == EyeShape::Line;
}
