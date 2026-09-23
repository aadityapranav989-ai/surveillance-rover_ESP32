#include <Arduino.h>
#include "motors.h"
#include "config.h"

void initMotors()
{
    // Configure PWM channels
    ledcSetup(LEFT_RPWM_CH, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(LEFT_LPWM_CH, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(RIGHT_RPWM_CH, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(RIGHT_LPWM_CH, PWM_FREQ, PWM_RESOLUTION);

    // Attach pins
    ledcAttachPin(LEFT_RPWM, LEFT_RPWM_CH);
    ledcAttachPin(LEFT_LPWM, LEFT_LPWM_CH);

    ledcAttachPin(RIGHT_RPWM, RIGHT_RPWM_CH);
    ledcAttachPin(RIGHT_LPWM, RIGHT_LPWM_CH);

    stopMotors();

    Serial.println("Motor Driver Initialized");
}

void setMotor(int leftSpeed, int rightSpeed)
{
    leftSpeed = constrain(leftSpeed, -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);

    // LEFT SIDE
    if (leftSpeed >= 0)
    {
        ledcWrite(LEFT_RPWM_CH, leftSpeed);
        ledcWrite(LEFT_LPWM_CH, 0);
    }
    else
    {
        ledcWrite(LEFT_RPWM_CH, 0);
        ledcWrite(LEFT_LPWM_CH, -leftSpeed);
    }

    // RIGHT SIDE
    if (rightSpeed >= 0)
    {
        ledcWrite(RIGHT_RPWM_CH, rightSpeed);
        ledcWrite(RIGHT_LPWM_CH, 0);
    }
    else
    {
        ledcWrite(RIGHT_RPWM_CH, 0);
        ledcWrite(RIGHT_LPWM_CH, -rightSpeed);
    }
}

void stopMotors()
{
    ledcWrite(LEFT_RPWM_CH, 0);
    ledcWrite(LEFT_LPWM_CH, 0);

    ledcWrite(RIGHT_RPWM_CH, 0);
    ledcWrite(RIGHT_LPWM_CH, 0);

    // Serial.println("STOPPED");
}
