#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "this_time.h"
// === GPS Config ===
// Change pins & serial port according to your board
#define GPS_RX_PIN 15 //use the pin no. labeled tx on neogps6m
#define GPS_TX_PIN 4 //use the pin no. labeled rx on neogps6m trust me
#define GPS_BAUD 9600

// TinyGPS++ object
TinyGPSPlus gps;

// HardwareSerial for GPS (use Serial1, Serial2 depending on board)
HardwareSerial GPSSerial(1);

// Variables to store last known coordinates
double currentLat = -1;
double currentLon = -1;
bool gpsListening = false;
unsigned long millisAtStart = 0;


float lastKnownLat = -1;
float lastKnownLon = -1;
bool getLocation()
{
  if (currentLat != -1 && currentLon != -1)
    return true;
  else
    return false;
}
void clearLocation()
{
  currentLat = currentLon = -1;
}



void listenToGPS()
{
  if (!gpsListening)
  {
    GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    gpsListening = true;
  }
  else
  {
    if (GPSSerial.available()){
      char r = GPSSerial.read();
      gps.encode(r);
    }

    if (gps.location.isUpdated()) {
      lastKnownLat = currentLat = gps.location.lat();
      lastKnownLon = currentLon = gps.location.lng();
      if (gps.date.isValid() && gps.time.isValid()) {
        millisAtStart = - millis() + gpsToUnixMillis(
                          gps.date.year(),
                          gps.date.month(),
                          gps.date.day(),
                          gps.time.hour(),
                          gps.time.minute(),
                          gps.time.second()
                        );
      }

    }
  }
}
// Stop GPS listening 
void stopListeningToGPS()
{
  if (gpsListening)
  {
    GPSSerial.end();
    gpsListening = false;
  }
}

// Clear GPS serial buffer
void clearGPSBuffer()
{
  while (GPSSerial.available())
  {
    GPSSerial.read();
  }
}
