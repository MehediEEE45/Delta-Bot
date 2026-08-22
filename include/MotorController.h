#pragma once

#include <Arduino.h>

enum class MotorCommand {
    Stop,
    Forward,
    Backward,
    Left,
    Right
};

class MotorController {
public:
    MotorController(uint8_t leftPwm, uint8_t leftIn1, uint8_t leftIn2, uint8_t rightPwm, uint8_t rightIn1, uint8_t rightIn2, uint8_t standby);
    void begin();
    void drive(MotorCommand command, uint8_t speed);
    void stop();
    void update(unsigned long now);
    const char* commandName() const;

private:
    uint8_t leftPwm_;
    uint8_t leftIn1_;
    uint8_t leftIn2_;
    uint8_t rightPwm_;
    uint8_t rightIn1_;
    uint8_t rightIn2_;
    uint8_t standby_;
    MotorCommand command_ = MotorCommand::Stop;
    uint8_t speed_ = 0;
    unsigned long lastCommandAt_ = 0;
    void setMotor(uint8_t pwm, uint8_t in1, uint8_t in2, int16_t value);
};
