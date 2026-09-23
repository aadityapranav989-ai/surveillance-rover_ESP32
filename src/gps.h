#pragma once

void initGPS();
void updateGPS();
bool gpsHasFix();
double gpsLatitude();
double gpsLongitude();
double gpsAltitudeMeters();
uint32_t gpsSatellites();