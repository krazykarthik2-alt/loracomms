#include <SPI.h>

// SPI pins for the LoRa module
#define LORA_SS    5   // Chip Select (CS)
#define LORA_SCK   18  // Clock
#define LORA_MISO  19  // Master In, Slave Out
#define LORA_MOSI  23  // Master Out, Slave In
#define LORA_RST   14  // Not used in this test, but good to define
#define LORA_DIO0  26  // Not used in this test, but good to define

// SPI settings
SPISettings loraSettings(10E6, MSBFIRST, SPI_MODE0);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting SPI Loopback Test...");

  // Initialize SPI bus as Master
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  
  // Set up the Chip Select pin
  pinMode(LORA_SS, OUTPUT);
  digitalWrite(LORA_SS, HIGH); // Deselect the slave device initially
}

void loop() {
  // Data to send
  byte dataToSend = 0xAA;
  byte receivedData;
  
  Serial.println("--------------------");
  Serial.print("Sending data: ");
  Serial.println(dataToSend, HEX);

  // Begin SPI transaction
  SPI.beginTransaction(loraSettings);
  digitalWrite(LORA_SS, LOW); // Select the slave device
  
  // Transfer a single byte
  receivedData = SPI.transfer(dataToSend);
  
  digitalWrite(LORA_SS, HIGH); // Deselect the slave device
  SPI.endTransaction();
  
  Serial.print("Received data: ");
  Serial.println(receivedData, HEX);

  // Check if the sent and received data match
  if (dataToSend == receivedData) {
    Serial.println("SPI loopback successful! _");
  } else {
    Serial.println("SPI loopback failed. _ Check wiring.");
  }
  
  delay(2000); // Wait for 2 seconds before the next test
}
