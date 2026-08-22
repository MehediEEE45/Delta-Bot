#include "MotorController.h"

MotorController::MotorController(uint8_t leftPwm, uint8_t leftIn1, uint8_t leftIn2, uint8_t rightPwm, uint8_t rightIn1, uint8_t rightIn2, uint8_t standby)
    : leftPwm_(leftPwm), leftIn1_(leftIn1), leftIn2_(leftIn2), rightPwm_(rightPwm), rightIn1_(rightIn1), rightIn2_(rightIn2), standby_(standby) {}

void MotorController::begin() {
    pinMode(leftPwm_, OUTPUT);
    pinMode(leftIn1_, OUTPUT);
    pinMode(leftIn2_, OUTPUT);
    pinMode(rightIn1_, OUTPUT);
    pinMode(rightIn2_, OUTPUT);
    pinMode(standby_, OUTPUT);
    digitalWrite(standby_, HIGH);
    stop();
    lastCommandAt_ = millis();
}

void MotorController::setMotor(uint8_t pwm, uint8_t in1, uint8_t in2, int16_t value) {
    if (value > 0) {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        analogWrite(pwm, value);
    } else if (value < 0) {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        analogWrite(pwm, -value);
    } else {
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        analogWrite(pwm, 0);
    }
}

void MotorController::drive(MotorCommand command, uint8_t speed) {
    command_ = command;
    speed_ = speed;
    lastCommandAt_ = millis();
    switch (command) {
        case MotorCommand::Forward:
            setMotor(leftPwm_, leftIn1_, leftIn2_, speed);
            setMotor(rightPwm_, rightIn1_, rightIn2_, speed);
            break;
        case MotorCommand::Backward:
            setMotor(leftPwm_, leftIn1_, leftIn2_, -speed);
            setMotor(rightPwm_, rightIn1_, rightIn2_, -speed);
            break;
        case MotorCommand::Left:
            setMotor(leftPwm_, leftIn1_, leftIn2_, -speed);
            setMotor(rightPwm_, rightIn1_, rightIn2_, speed);
            break;
        case MotorCommand::Right:
            setMotor(leftPwm_, leftIn1_, leftIn2_, speed);
            setMotor(rightPwm_, rightIn1_, rightIn2_, -speed);
            break;
        default:
            stop();
            break;
    }
}

void MotorController::update(unsigned long now) {
    if (command_ != MotorCommand::Stop && now - lastCommandAt_ >= 1000) stop();
}

void MotorController::stop() {
    command_ = MotorCommand::Stop;
    speed_ = 0;
    setMotor(leftPwm_, leftIn1_, leftIn2_, 0);
    setMotor(rightPwm_, rightIn1_, rightIn2_, 0);
}

const char* MotorController::commandName() const {
    switch (command_) {
        case MotorCommand::Forward: return "forward";
        case MotorCommand::Backward: return "backward";
        case MotorCommand::Left: return "left";
        case MotorCommand::Right: return "right";
        default: return "stop";
    }
}
