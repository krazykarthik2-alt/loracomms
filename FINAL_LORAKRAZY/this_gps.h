#include <TinyGPS++.h>
#include <HardwareSerial.h>

// === GPS Config ===
// Change pins & serial port according to your board
#define GPS_RX_PIN 4
#define GPS_TX_PIN 15
#define GPS_BAUD 9600

// TinyGPS++ object
TinyGPSPlus gps;

// HardwareSerial for GPS (use Serial1, Serial2 depending on board)
HardwareSerial GPSSerial(1);

// Variables to store last known coordinates
double currentLat = 0.0;
double currentLon = 0.0;
bool gpsListening = false;

// Start GPS listening
void listenToGPS()
{
  if (!gpsListening)
  {
    GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    gpsListening = true;
  }
  else
  {
    if (GPSSerial.available())
      gps.encode(GPSSerial.read());
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

// Try to get a location fix (non-blocking)

float myLat = 17.385;
float myLon = 78.486;
bool getLocation()
{
  if (myLat != -1 && myLon != -1)
    return true;
  else
    return false;
}
void clearLocation()
{
  myLat = myLon = -1;
}

// Send location as LoRa beacon
