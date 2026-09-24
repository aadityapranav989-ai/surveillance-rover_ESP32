#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "wifi_link.h"

static bool fallbackAp = false;
static bool wasConnected = false;
static unsigned long disconnectedSince = 0;

static void startFallbackAp()
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_GATEWAY, WIFI_AP_SUBNET);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CLIENTS);
    fallbackAp = true;
    Serial.printf("Wi-Fi: cannot join %s; own network %s is open at %s\n", ROVER_WIFI_SSID, WIFI_AP_SSID,
                  WiFi.softAPIP().toString().c_str());
}

static void stopFallbackAp()
{
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    fallbackAp = false;
    Serial.printf("Wi-Fi: back on %s; closed %s\n", ROVER_WIFI_SSID, WIFI_AP_SSID);
}

void initWifi()
{
    WiFi.mode(WIFI_STA);
    // Stay awake: modem sleep delays incoming drive commands by up to ~100 ms.
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.config(ROVER_STATION_IP, ROVER_GATEWAY_IP, ROVER_SUBNET);
    WiFi.begin(ROVER_WIFI_SSID, ROVER_WIFI_PASSWORD);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    disconnectedSince = millis();
    Serial.printf("Wi-Fi: joining %s as %s\n", ROVER_WIFI_SSID, ROVER_STATION_IP.toString().c_str());
}

void updateWifi()
{
    bool connected = WiFi.status() == WL_CONNECTED;
    unsigned long now = millis();
    if (connected && !wasConnected)
    {
        Serial.printf("Wi-Fi: joined %s, address %s, signal %d dBm\n", ROVER_WIFI_SSID,
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        if (fallbackAp)
            stopFallbackAp();
    }
    if (!connected && wasConnected)
        disconnectedSince = now;
    if (!connected && !fallbackAp && now - disconnectedSince > FALLBACK_AP_AFTER_MS)
        startFallbackAp();
    wasConnected = connected;
}

bool wifiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

bool fallbackApActive()
{
    return fallbackAp;
}

String wifiAddress()
{
    return wifiConnected() ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
}
