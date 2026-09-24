#include <Arduino.h>

#include "motors.h"
#include "serial_cmd.h"
#include "watchdog.h"
#include "gps.h"
#include "web_server.h"
#include "diagnostics.h"
#include "lcd.h"
#include "udp_control.h"

// Motion timer
bool motionActive = false;

unsigned long motionStart = 0;
unsigned long motionDuration = 0;

void setup()
{
    Serial.begin(115200);

    Serial.println("ESP32 Robot Started");
    Serial.print("Last reset reason: ");
    Serial.println(resetReasonName());

    initMotors();
    initLcd();
    initGPS();
    initWebServer();
    initUdpControl();

    watchdogInit();
}

void loop()
{
    processSerial();
    processWebServer();
    processUdpControl();
    updateGPS();

    // Automatic stop after duration, easing down instead of cutting power.
    if (motionActive)
    {
        if (millis() - motionStart >= motionDuration)
        {
            easeToStop();
            motionActive = false;
        }
    }
    updateMotors();
    updateLcd();

    watchdogUpdate();
}