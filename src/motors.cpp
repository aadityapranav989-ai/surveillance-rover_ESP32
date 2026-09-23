#include <Arduino.h>
#include "motors.h"
#include "config.h"

// What each side is driving now and what it is ramping towards (-255..255).
static float currentLeft = 0, currentRight = 0;
static int targetLeft = 0, targetRight = 0;
static unsigned long lastRampUpdate = 0;

static void writeSide(int forwardChannel, int reverseChannel, int speed)
{
    if (speed >= 0)
    {
        ledcWrite(forwardChannel, speed);
        ledcWrite(reverseChannel, 0);
    }
    else
    {
        ledcWrite(forwardChannel, 0);
        ledcWrite(reverseChannel, -speed);
    }
}

static void writeOutputs()
{
    writeSide(LEFT_RPWM_CH, LEFT_LPWM_CH, (int)roundf(currentLeft));
    writeSide(RIGHT_RPWM_CH, RIGHT_LPWM_CH, (int)roundf(currentRight));
}

static float approach(float current, int target, float maxStep)
{
    if (current < target)
        return min(current + maxStep, (float)target);
    return max(current - maxStep, (float)target);
}

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
    lastRampUpdate = millis();

    Serial.println("Motor Driver Initialized");
}

void setMotor(int leftSpeed, int rightSpeed)
{
    // The motors ramp to these speeds in updateMotors() instead of jumping.
    targetLeft = constrain(leftSpeed, -255, 255);
    targetRight = constrain(rightSpeed, -255, 255);
}

void updateMotors()
{
    unsigned long now = millis();
    unsigned long elapsed = now - lastRampUpdate;
    if (elapsed == 0)
        return;
    lastRampUpdate = now;

    // A full speed change (0 -> 255) takes MOTOR_RAMP_MS.
    float maxStep = elapsed * 255.0f / MOTOR_RAMP_MS;
    currentLeft = approach(currentLeft, targetLeft, maxStep);
    currentRight = approach(currentRight, targetRight, maxStep);
    writeOutputs();
}

void easeToStop()
{
    setMotor(0, 0);
}

void stopMotors()
{
    // Immediate: used for STOP and the watchdog.
    targetLeft = targetRight = 0;
    currentLeft = currentRight = 0;
    writeOutputs();
}
