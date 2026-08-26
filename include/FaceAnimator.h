#pragma once

#include <Arduino.h>
#include "AppMode.h"

enum class Emotion; // defined in FaceRenderer.h

// How the eye is drawn. Shapes are discrete, so they switch at the start of a
// morph while the geometry below eases across it.
enum class EyeShape : uint8_t {
    Block,     // rounded rectangle, the neutral eye
    HappyArc,  // block with an arc carved out of the bottom: the "^ ^" squint
    Heart,
    Star,
    Shades,    // one visor bar spanning both eyes
    Hollow,    // ring, for surprise
    Line       // closed lid
};

// The interpolatable half of an expression. Every field is a float so two
// expressions can be mixed by simple lerp.
struct FaceParams {
    float eyeW      = 27.0f;
    float eyeH      = 21.0f;
    float eyeRadius = 6.0f;
    float eyeGap    = 66.0f;  // centre-to-centre distance
    float eyeY      = 26.0f;  // centre line
    float browLift  = 0.0f;   // px above the eye; 0 hides the brow
    float browAngle = 0.0f;   // +ve raises the inner end (sad), -ve lowers it (angry)
    float mouthW    = 22.0f;
    float mouthCurve = 0.0f;  // +ve smile, -ve frown
    float mouthOpen = 0.0f;   // 0 = line, >0 = filled ellipse of this height
    float mouthY    = 48.0f;
};

// Everything the renderer needs for one frame. Produced by FaceAnimator so the
// drawing code holds no animation state of its own.
struct FaceFrame {
    FaceParams p;
    EyeShape shape = EyeShape::Block;
    float openness = 1.0f;   // blink multiplier applied to eyeH
    float offsetX  = 0.0f;   // idle drift + burn-in drift
    float offsetY  = 0.0f;
    float gazeX    = 0.0f;   // pupil offset within the eye
    float gazeY    = 0.0f;
    bool  showPupil = true;
    bool  showZzz   = false;
};

// Drives the face: eases between expressions, blinks, drifts, and glances
// around. Holds no reference to the display, so it can be stepped with
// synthetic timestamps in a native unit test.
class FaceAnimator {
public:
    void begin(unsigned long now);
    void update(Emotion emotion, unsigned long now);
    const FaceFrame& frame() const { return frame_; }

    // Point the eyes somewhere deliberately, in the range [-1, 1] on each
    // axis. Used by RC mode to lean the gaze into a turn. Cleared by
    // releaseLook(), after which idle saccades resume.
    void lookAt(float x, float y);
    void releaseLook();

private:
    static FaceParams paramsFor(Emotion emotion);
    static EyeShape shapeFor(Emotion emotion);
    void stepBlink(unsigned long now);
    void stepGaze(unsigned long now);

    FaceFrame frame_;
    FaceParams from_;
    FaceParams to_;
    Emotion current_ = static_cast<Emotion>(0);
    unsigned long morphStartedAt_ = 0;
    bool started_ = false;

    unsigned long nextBlinkAt_ = 0;
    unsigned long blinkStartedAt_ = 0;
    bool blinking_ = false;
    bool queueSecondBlink_ = false;

    float gazeFromX_ = 0.0f, gazeFromY_ = 0.0f;
    float gazeToX_ = 0.0f, gazeToY_ = 0.0f;
    unsigned long saccadeStartedAt_ = 0;
    unsigned long nextSaccadeAt_ = 0;
    bool lookHeld_ = false;
};
