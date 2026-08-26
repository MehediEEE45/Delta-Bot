#include "MotorController.h"
#include "Config.h"

MotorController::MotorController(uint8_t leftPwm, uint8_t leftIn1, uint8_t leftIn2, uint8_t rightPwm, uint8_t rightIn1, uint8_t rightIn2, uint8_t standby)
    : leftPwm_(leftPwm), leftIn1_(leftIn1), leftIn2_(leftIn2), rightPwm_(rightPwm), rightIn1_(rightIn1), rightIn2_(rightIn2), standby_(standby) {}

void MotorController::begin() {
    pinMode(leftPwm_, OUTPUT);
    pinMode(leftIn1_, OUTPUT);
    pinMode(leftIn2_, OUTPUT);
    pinMode(rightPwm_, OUTPUT);
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

void MotorController::applyCommand(MotorCommand command, uint8_t speed) {
    const int16_t s = static_cast<int16_t>(speed);
    switch (command) {
        case MotorCommand::Forward:
            setMotor(leftPwm_, leftIn1_, leftIn2_, s);
            setMotor(rightPwm_, rightIn1_, rightIn2_, s);
            break;
        case MotorCommand::Backward:
            setMotor(leftPwm_, leftIn1_, leftIn2_, -s);
            setMotor(rightPwm_, rightIn1_, rightIn2_, -s);
            break;
        case MotorCommand::Left:
            setMotor(leftPwm_, leftIn1_, leftIn2_, -s);
            setMotor(rightPwm_, rightIn1_, rightIn2_, s);
            break;
        case MotorCommand::Right:
            setMotor(leftPwm_, leftIn1_, leftIn2_, s);
            setMotor(rightPwm_, rightIn1_, rightIn2_, -s);
            break;
        default:
            setMotor(leftPwm_, leftIn1_, leftIn2_, 0);
            setMotor(rightPwm_, rightIn1_, rightIn2_, 0);
            break;
    }
}

void MotorController::drive(MotorCommand command, uint8_t speed) {
    shakeStep_ = 0; // an explicit drive command cancels a shake in progress
    command_ = command;
    speed_ = command == MotorCommand::Stop ? 0 : speed;
    lastCommandAt_ = millis();
    applyCommand(command_, speed_);
}

void MotorController::startShake(unsigned long now) {
    shakeStep_ = 1;
    shakeStepAt_ = now;
    command_ = MotorCommand::Left;
    speed_ = Config::MOTOR_SHAKE_SPEED;
    lastCommandAt_ = now;
    applyCommand(MotorCommand::Left, Config::MOTOR_SHAKE_SPEED);
}

bool MotorController::isShaking() const {
    return shakeStep_ != 0;
}

void MotorController::update(unsigned long now) {
    if (shakeStep_ != 0) {
        if (now - shakeStepAt_ < Config::MOTOR_SHAKE_LEG_MS) return;
        shakeStepAt_ = now;
        lastCommandAt_ = now;
        if (shakeStep_ == 1) {
            shakeStep_ = 2;
            command_ = MotorCommand::Right;
            applyCommand(MotorCommand::Right, Config::MOTOR_SHAKE_SPEED);
        } else {
            shakeStep_ = 0;
            stop();
        }
        return;
    }

    if (command_ != MotorCommand::Stop && now - lastCommandAt_ >= Config::MOTOR_WATCHDOG_MS) stop();
}

void MotorController::stop() {
    shakeStep_ = 0;
    command_ = MotorCommand::Stop;
    speed_ = 0;
    applyCommand(MotorCommand::Stop, 0);
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
