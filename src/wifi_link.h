#pragma once

#include <Arduino.h>

// The ESP32 joins the Raspberry Pi's rover Wi-Fi (ROVER_WIFI_SSID) with a fixed
// address. If it cannot join for FALLBACK_AP_AFTER_MS, it also opens its own
// "ESP32-Robot" network (the old setup) so it is never unreachable; that network
// closes again once the Pi's network is back.
void initWifi();
void updateWifi();

bool wifiConnected();
bool fallbackApActive();
String wifiAddress();  // the address the rover is reachable on
