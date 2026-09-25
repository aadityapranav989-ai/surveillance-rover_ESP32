#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Wire.h>

#include "config.h"
#include "gps.h"
#include "lcd.h"
#include "wifi_link.h"

static LiquidCrystal_I2C *lcd = nullptr;
static String shownLine1, shownLine2;
static unsigned long lastPiMessage = 0;
static bool piMessageSeen = false;
static unsigned long lastFallbackUpdate = 0;
static portMUX_TYPE queueLock = portMUX_INITIALIZER_UNLOCKED;
static char queued1[17], queued2[17];
static bool queuedText = false;

static String fit(const String &text)
{
    // Exactly 16 printable ASCII characters, so old text is always overwritten.
    String out;
    for (unsigned int i = 0; i < text.length() && out.length() < LCD_COLUMNS; i++)
    {
        char c = text[i];
        out += (c >= 32 && c < 127) ? c : '?';
    }
    while (out.length() < LCD_COLUMNS)
        out += ' ';
    return out;
}

static void draw(const String &line1, const String &line2)
{
    if (lcd == nullptr)
        return;
    String a = fit(line1), b = fit(line2);
    if (a != shownLine1)
    {
        lcd->setCursor(0, 0);
        lcd->print(a);
        shownLine1 = a;
    }
    if (b != shownLine2)
    {
        lcd->setCursor(0, 1);
        lcd->print(b);
        shownLine2 = b;
    }
}

static bool i2cDevicePresent(uint8_t address)
{
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

void initLcd()
{
    Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
    // Backpacks ship at 0x27 (PCF8574) or 0x3F (PCF8574A).
    uint8_t address = 0;
    for (uint8_t candidate : {LCD_I2C_ADDRESS, (uint8_t)0x27, (uint8_t)0x3F})
    {
        if (candidate != 0 && i2cDevicePresent(candidate))
        {
            address = candidate;
            break;
        }
    }
    if (address == 0)
    {
        Serial.println("LCD: no I2C display found on GPIO21/22 (checked 0x27 and 0x3F)");
        return;
    }
    lcd = new LiquidCrystal_I2C(address, LCD_COLUMNS, 2);
    lcd->init();
    lcd->backlight();
    Serial.printf("LCD: found at 0x%02X\n", address);
    draw("ROVER SENTRY", "Starting...");
}

void lcdShow(const String &line1, const String &line2)
{
    lastPiMessage = millis();
    piMessageSeen = true;
    draw(line1, line2);
}

void lcdQueue(const String &line1, const String &line2)
{
    portENTER_CRITICAL(&queueLock);
    strlcpy(queued1, line1.c_str(), sizeof(queued1));
    strlcpy(queued2, line2.c_str(), sizeof(queued2));
    queuedText = true;
    portEXIT_CRITICAL(&queueLock);
}

void updateLcd()
{
    if (queuedText)
    {
        char line1[17], line2[17];
        portENTER_CRITICAL(&queueLock);
        memcpy(line1, queued1, sizeof(line1));
        memcpy(line2, queued2, sizeof(line2));
        queuedText = false;
        portEXIT_CRITICAL(&queueLock);
        lcdShow(line1, line2);
    }
    if (lcd == nullptr)
        return;
    unsigned long now = millis();
    bool piQuiet = !piMessageSeen || now - lastPiMessage > LCD_PI_TIMEOUT_MS;
    if (!piQuiet || now - lastFallbackUpdate < 1000)
        return;
    lastFallbackUpdate = now;
    // No recent message from the Pi: show the rover's own status instead of a stale message.
    String line2 = String("Pi offline ") + (gpsHasFix() ? "GPS" : "noGPS");
    if ((now / 3000) % 2 == 1)
        line2 = wifiConnected() ? String("IP ") + wifiAddress() : String("No Wi-Fi");
    draw("ROVER SENTRY", line2);
}

bool lcdFound()
{
    return lcd != nullptr;
}
