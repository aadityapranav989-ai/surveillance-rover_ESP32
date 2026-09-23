#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "config.h"
#include "lcd.h"

static LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);
static unsigned long lastLCDUpdate = 0;
static bool lcdReady = false;
static unsigned long lastRemoteStatus = 0;
static char remoteState[17] = "PI LINK READY";
static char remoteDetail[17] = "Waiting...";

static void printLine(uint8_t row, const char *text)
{
    lcd.setCursor(0, row);
    lcd.print(text);
    for (size_t length = strlen(text); length < LCD_COLUMNS; ++length)
        lcd.print(' ');
}

static void copyLCDText(char *destination, const char *source)
{
    strncpy(destination, source, LCD_COLUMNS);
    destination[LCD_COLUMNS] = '\0';
}

void initLCD()
{
    Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
    lcd.init();
    lcd.backlight();
    lcdReady = true;

    printLine(0, "ESP32 Rover");
    printLine(1, "Starting...");
}

void updateLCD(bool motionActive)
{
    if (!lcdReady || millis() - lastLCDUpdate < 500)
        return;

    lastLCDUpdate = millis();
    if (millis() - lastRemoteStatus < 3000)
    {
        printLine(0, remoteState);
        printLine(1, remoteDetail);
    }
    else
    {
        printLine(0, motionActive ? "Status: MOVING" : "Status: READY");
        printLine(1, motionActive ? "Drive active" : "IP 192.168.4.1");
    }
}

void setLCDRemoteStatus(const char *state, const char *detail)
{
    copyLCDText(remoteState, state);
    copyLCDText(remoteDetail, detail);
    lastRemoteStatus = millis();
}