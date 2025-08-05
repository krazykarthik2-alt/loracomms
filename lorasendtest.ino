#include <SPI.h>
#include <LoRa.h>

// LoRa pins
#define SS      5
#define RST     14
#define DIO0    26

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check connections.");
    while (true);
  }

  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(62.5E3);
  LoRa.setCodingRate4(8);
  LoRa.setSyncWord(0x22);

  Serial.println("LoRa Transmitter started");
}

void loop() {
  String msg = "1234";  // dummy test payload

  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();

  Serial.println("Sent: " + msg);
  delay(2000); // send every 2 seconds
}
