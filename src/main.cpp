#include <Arduino.h>

#include "motors.h"
#include "serial_cmd.h"
#include "watchdog.h"
#include "gps.h"
#include "web_server.h"

// Motion timer
bool motionActive = false;

unsigned long motionStart = 0;
unsigned long motionDuration = 0;

void setup()
{
    Serial.begin(115200);

    Serial.println("ESP32 Robot Started");

    initMotors();
    initGPS();
    initWebServer();

    watchdogInit();
}

void loop()
{
    processSerial();
    processWebServer();
    updateGPS();

    // Automatic stop after duration
    if (motionActive)
    {
        if (millis() - motionStart >= motionDuration)
        {
            stopMotors();
            motionActive = false;
        }
    }

    watchdogUpdate();
}