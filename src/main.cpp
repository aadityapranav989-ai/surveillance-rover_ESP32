#include <Arduino.h>

#include "motors.h"
#include "serial_cmd.h"
#include "watchdog.h"
#include "gps.h"
#include "web_server.h"
#include "diagnostics.h"
#include "lcd.h"
#include "udp_control.h"
#include "wifi_link.h"
#include "control.h"

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
    initWifi();
    initWebServer();
    initUdpControl();

    watchdogInit();
    initControlTask();
}

void loop()
{
    // Motors, UDP drive commands and the watchdog run in the control task
    // (control.cpp); this loop only handles work that may be slow.
    updateWifi();
    processSerial();
    processWebServer();
    updateGPS();
    updateLcd();
    delay(1);
}