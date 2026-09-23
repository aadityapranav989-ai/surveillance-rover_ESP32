#pragma once

#include <Arduino.h>

// 16x2 character LCD on an I2C backpack (PCF8574).
void initLcd();

// Shows two lines sent by the Raspberry Pi (each cut to 16 characters).
void lcdShow(const String &line1, const String &line2);

// Falls back to the rover's own status when the Pi has gone quiet; call every loop.
void updateLcd();

bool lcdFound();
