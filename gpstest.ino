  #include <U8g2lib.h>
  #include <Wire.h>
  #include <HardwareSerial.h>
  #include <TinyGPS++.h>
  #include <QMC5883LCompass.h> // Include the compass library
  
  // --- Pin Definitions ---
  // I2C pins for OLED and Compass
  #define I2C_SDA 21
  #define I2C_SCL 22
  
  // GPS Pin Definitions
  // IMPORTANT: GPS TX connects to ESP32 RX, GPS RX connects to ESP32 TX
  #define GPS_RX_PIN 15  // Connect GPS TX (transmit) to ESP32 RX (receive) on GPIO4
  #define GPS_TX_PIN 4 // Connect GPS RX (receive) to ESP32 TX (transmit) on GPIO15
  
  // --- Global Objects ---
  // OLED display object (SH1106, 128x64, I2C, no reset pin)
  U8G2_SH1106_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
  
  // TinyGPS++ object for parsing GPS data
  TinyGPSPlus gps;
  
  // HardwareSerial object for GPS communication (using UART1)
  HardwareSerial gpsSerial(1); // ESP32 has 3 hardware serial ports: Serial (UART0), Serial1 (UART1), Serial2 (UART2)
  
  // Compass object
  QMC5883LCompass compass;
  
  void setup() {
    // Initialize Serial Monitor for debugging output
    Serial.begin(115200);
    Serial.println("Starting ESP32 All-in-One Test...");
  
    // --- I2C Initialization (for OLED and Compass) ---
    // Wire.begin() initializes the I2C bus. u8g2.begin() calls Wire.begin() internally,
    // so explicitly calling Wire.begin() here is optional but good for clarity
    // if other I2C devices are initialized separately.
    Wire.begin(I2C_SDA, I2C_SCL);
  
    // Initialize OLED display
    u8g2.begin();
    u8g2.setFont(u8g2_font_6x12_tf); // Set a small font for more text on screen
    u8g2.clearBuffer();             // Clear the display buffer
    u8g2.sendBuffer();              // Send the empty buffer to the display
  
    // Display initial message on OLED
    u8g2.firstPage();
    do {
      u8g2.setCursor(0, 10);
      u8g2.print("Initializing...");
      u8g2.setCursor(0, 30);
      u8g2.print("GPS, Compass, OLED");
    } while (u8g2.nextPage());
    Serial.println("OLED initialized.");
  
    // --- GPS Initialization ---
    // Initialize the GPS hardware serial port at 9600 baud (common for NEO-6M)
    gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("GPS serial initialized.");
  
    // --- Compass Initialization ---
  compass.init();
    Serial.println("Compass initialized.");
  
    // Display "Waiting for signal..." after all initializations
    u8g2.firstPage();
    do {
      u8g2.setCursor(0, 10);
      u8g2.print("All modules ready!");
      u8g2.setCursor(0, 35);
      u8g2.print("Waiting for GPS signal...");
    } while (u8g2.nextPage());
    Serial.println("Setup complete. Entering loop.");
  }
  
  void loop() {
    // --- GPS Data Processing ---
    // Read any available data from the GPS module and feed it to TinyGPS++
    while (gpsSerial.available() > 0) {
      char gpsChar = gpsSerial.read();
      if (gps.encode(gpsChar)) {
        // Data was successfully parsed by TinyGPS++
      }
      // Echo raw GPS data to Serial Monitor for debugging
      Serial.write(gpsChar);
    }
  
    // --- Display Update Logic ---
    // Update the display if new GPS location data is available
    if (gps.location.isUpdated()) {
      u8g2.firstPage();
      do {
        u8g2.setCursor(0, 10);
        u8g2.print("GPS Data");
  
        u8g2.setCursor(0, 25);
        u8g2.print("Lat: ");
        u8g2.print(gps.location.lat(), 6); // Latitude with 6 decimal places
  
        u8g2.setCursor(0, 40);
        u8g2.print("Lon: ");
        u8g2.print(gps.location.lng(), 6); // Longitude with 6 decimal places
  
        u8g2.setCursor(0, 55);
        u8g2.print("Sats: ");
        u8g2.print(gps.satellites.value()); // Number of satellites
  
        // --- IST Time Calculation and Display ---
        // Get UTC time from GPS
        int utcHour = gps.time.hour();
        int utcMinute = gps.time.minute();
        int utcSecond = gps.time.second();
  compass.read();
      int heading = compass.getAzimuth(); // Get azimuth in degrees
    
        // Add 5 hours and 30 minutes for GMT+5:30 (Indian Standard Time)
        int istMinute = utcMinute + 30;
        int istHour = utcHour + 5;
        int istSecond = utcSecond; // Seconds don't change
  
        // Handle minute roll-over (if minutes go >= 60, add an hour)
        if (istMinute >= 60) {
          istMinute -= 60;
          istHour++;
        }
  
        // Handle hour roll-over (if hours go >= 24, reset to 0)
        if (istHour >= 24) {
          istHour -= 24;
        }
          u8g2.setCursor(50, 0);
        u8g2.print(heading);
        u8g2.print(" deg");
  
  
    
        u8g2.setCursor(60, 55); // Adjust position to fit on the same line as Sats
        u8g2.print(">"); // Separator
        if (istHour < 10) u8g2.print("0"); // Add leading zero for hours
        u8g2.print(istHour);
        u8g2.print(":");
        if (istMinute < 10) u8g2.print("0"); // Add leading zero for minutes
        u8g2.print(istMinute);
        u8g2.print(":");
        if (istSecond < 10) u8g2.print("0"); // Add leading zero for seconds
        u8g2.print(istSecond);
  
      } while (u8g2.nextPage());
    } 
  }
