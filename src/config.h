#pragma once

#include <Arduino.h>

// ---------------- LEFT BTS7960 ----------------
const int LEFT_RPWM = 25;
const int LEFT_LPWM = 26;

// ---------------- RIGHT BTS7960 ----------------
const int RIGHT_RPWM = 27;
const int RIGHT_LPWM = 14;

// Motion calibration variables
const float DISTANCE_MS_PER_CM = 15.3f;
const float TURN_MS_PER_DEGREE = 11.4f;
const float SPEED_INPUT_SCALE = 1.0f;

// PWM Settings
const uint32_t PWM_FREQ = 1000;
const uint8_t PWM_RESOLUTION = 8;

// PWM Channels
const int LEFT_RPWM_CH = 0;
const int LEFT_LPWM_CH = 1;
const int RIGHT_RPWM_CH = 2;
const int RIGHT_LPWM_CH = 3;

// GY-NEO6MV2 GPS module on ESP32 UART2
const int GPS_RX_PIN = 16; // Connect to GPS TX
const int GPS_TX_PIN = 17; // Connect to GPS RX
const uint32_t GPS_BAUD_RATE = 9600;

// 16x2 LCD with an I2C backpack. Most backpacks use 0x27; some use 0x3F.
const uint8_t LCD_I2C_ADDRESS = 0x27;
const int LCD_SDA_PIN = 21;
const int LCD_SCL_PIN = 22;
const uint8_t LCD_COLUMNS = 16;
const uint8_t LCD_ROWS = 2;

// Longest single movement the ESP32 will run. A command that asks for more is
// shortened to this, so a bad value can never keep the motors running.
const unsigned long MAX_MOTION_MS = 3000;

// Time for a motor to ramp from stopped to full speed (and back). Softens every
// start, stop and change of direction; STOP still cuts power immediately.
const unsigned long MOTOR_RAMP_MS = 250;

// Self-contained rover Wi-Fi network. The ESP32 is the access point.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Missing src/secrets.h: copy src/secrets.example.h to src/secrets.h and set the Wi-Fi password"
#endif

static_assert(sizeof(ROVER_WIFI_PASSWORD) - 1 >= 8 &&
                  sizeof(ROVER_WIFI_PASSWORD) - 1 <= 63,
              "ROVER_WIFI_PASSWORD must be 8-63 characters (WPA2)");

const char *const WIFI_AP_SSID = "ESP32-Robot";
const char *const WIFI_AP_PASSWORD = ROVER_WIFI_PASSWORD;
const IPAddress WIFI_AP_IP(192, 168, 4, 1);
const IPAddress WIFI_AP_GATEWAY(192, 168, 4, 1);
const IPAddress WIFI_AP_SUBNET(255, 255, 255, 0);
// Wi-Fi channel 1-13. If the rover Wi-Fi drops near other networks, try 6 or 11.
const int WIFI_AP_CHANNEL = 1;
// The ESP32 default is 4 devices; the Pi, laptop and a couple of phones can exceed that.
const int WIFI_AP_MAX_CLIENTS = 8;