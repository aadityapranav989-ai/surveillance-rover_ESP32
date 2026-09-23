#include <Arduino.h>
#include "watchdog.h"
#include "motors.h"

extern bool motionActive;

static unsigned long lastCommandTime = 0;
static bool watchdogTriggered = false;

// Stop after 500 ms without commands
const unsigned long WATCHDOG_TIMEOUT = 5000;

void watchdogInit()
{
    lastCommandTime = millis();
    watchdogTriggered = false;
}

void watchdogKick()
{
    lastCommandTime = millis();
    watchdogTriggered = false;
}

void watchdogUpdate()
{
    unsigned long now = millis();

    if (motionActive)
    {
        lastCommandTime = now;
        watchdogTriggered = false;
        return;
    }

    if (now - lastCommandTime > WATCHDOG_TIMEOUT)
    {
        if (!watchdogTriggered)
        {
            Serial.println("Watchdog timeout: stopping motors");
            watchdogTriggered = true;
        }
        stopMotors();
    }
}