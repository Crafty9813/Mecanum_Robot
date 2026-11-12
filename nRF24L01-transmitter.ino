#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(7, 8);

const byte address[6] = "00001";

// Joy 1
const int joy1XPin = A0; // Strafe
const int joy1YPin = A3; // Translate

// Joy 2
const int joy2XPin = A2; // Rot.

struct Data {
  int joy1X;
  int joy1Y;
  int joy2X;
};

Data data;

void setup() {
  Serial.begin(9600);

  radio.begin();

  radio.openWritingPipe(address);

  radio.setPALevel(RF24_PA_MIN);

  radio.stopListening();
}

void loop() {
  data.joy1X = analogRead(joy1XPin);
  data.joy1Y = analogRead(joy1YPin);
  data.joy2X = analogRead(joy2XPin);

  radio.write(&data, sizeof(data));

  // 20 ms = 50 Hz
  delay(20);
}
