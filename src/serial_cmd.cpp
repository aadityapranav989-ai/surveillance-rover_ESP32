#include <Arduino.h>
#include "serial_cmd.h"
#include "motors.h"
#include "watchdog.h"
#include "config.h"

// Motion variables from main.cpp
extern bool motionActive;
extern unsigned long motionStart;
extern unsigned long motionDuration;

static int calibrateSpeed(int inputSpeed)
{
    float scaledSpeed = inputSpeed * SPEED_INPUT_SCALE;
    return constrain((int)round(scaledSpeed), -255, 255);
}

static unsigned long calibrateDistance(float distanceCm)
{
    return (unsigned long)round(distanceCm * DISTANCE_MS_PER_CM);
}

static unsigned long calibrateTurn(float angleDeg)
{
    return (unsigned long)round(angleDeg * TURN_MS_PER_DEGREE);
}

void startMotion(int left, int right, unsigned long duration)
{
    watchdogKick();

    setMotor(calibrateSpeed(left), calibrateSpeed(right));

    motionActive = true;
    motionStart = millis();
    motionDuration = duration;
}

bool executeCommand(const String &cmd)
{
    char action[20];
    int speed;
    float value;

    if (sscanf(cmd.c_str(), "%s %d %f", action, &speed, &value) == 3)
    {
        String act = String(action);

        if (act == "FORWARD")
        {
            unsigned long duration = calibrateDistance(value);
            startMotion(speed, speed, duration);
            return true;
        }

        if (act == "BACKWARD")
        {
            unsigned long duration = calibrateDistance(value);
            startMotion(-speed, -speed, duration);
            return true;
        }

        if (act == "LEFT")
        {
            unsigned long duration = calibrateTurn(value);
            startMotion(-speed, speed, duration);
            return true;
        }

        if (act == "RIGHT")
        {
            unsigned long duration = calibrateTurn(value);
            startMotion(speed, -speed, duration);
            return true;
        }
    }

    if (cmd == "STOP")
    {
        stopMotors();
        motionActive = false;
        watchdogKick();
        return true;
    }

    return false;
}

void processSerial()
{
    if (!Serial.available())
        return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (!executeCommand(cmd))
        Serial.println("INVALID COMMAND");
}