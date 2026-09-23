#include <Arduino.h>
#include <TinyGPSPlus.h>

#include "config.h"
#include "gps.h"

static HardwareSerial gpsSerial(2);
static TinyGPSPlus gps;
static unsigned long lastLocationPrint = 0;
static unsigned long lastNoFixPrint = 0;

void initGPS()
{
    gpsSerial.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("GPS Initialized (UART2, 9600 baud)");
}

void updateGPS()
{
    while (gpsSerial.available() > 0)
    {
        gps.encode(gpsSerial.read());
    }

    unsigned long now = millis();

    if (gps.location.isValid() && now - lastLocationPrint >= 1000)
    {
        lastLocationPrint = now;

        Serial.print("GPS: ");
        Serial.print(gps.location.lat(), 6);
        Serial.print(", ");
        Serial.print(gps.location.lng(), 6);
        Serial.print(" | Alt: ");
        Serial.print(gps.altitude.meters(), 1);
        Serial.print(" m | Satellites: ");
        Serial.println(gps.satellites.value());
    }
    else if (!gps.location.isValid() && now - lastNoFixPrint >= 5000)
    {
        lastNoFixPrint = now;
        Serial.println("GPS: waiting for location fix...");
    }
}

bool gpsHasFix()
{
    return gps.location.isValid();
}

double gpsLatitude()
{
    return gps.location.lat();
}

double gpsLongitude()
{
    return gps.location.lng();
}

double gpsAltitudeMeters()
{
    return gps.altitude.meters();
}

uint32_t gpsSatellites()
{
    return gps.satellites.value();
}