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

// Self-contained rover Wi-Fi network. The ESP32 is the access point.
const char *const WIFI_AP_SSID = "ESP32-Robot";
const char *const WIFI_AP_PASSWORD = "robot123";
const IPAddress WIFI_AP_IP(192, 168, 4, 1);
const IPAddress WIFI_AP_GATEWAY(192, 168, 4, 1);
const IPAddress WIFI_AP_SUBNET(255, 255, 255, 0);