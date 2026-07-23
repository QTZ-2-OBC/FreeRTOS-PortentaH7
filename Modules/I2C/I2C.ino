/*
  SAMD21 I2C Master Example
  ---------------------------
  A SAMD21 board (Arduino Zero, MKR series, etc.) acts as I2C master,
  periodically sending a command byte to another microcontroller (the
  slave) and reading back a one-byte response. Uses the official
  Arduino "Wire" library and checks for transmission/reception errors.

  Wiring:
    SAMD21 SDA  -- Slave SDA
    SAMD21 SCL  -- Slave SCL
    GND         -- GND (common ground required)
    Pull-up resistors (2.2k-4.7k) on SDA/SCL to 3.3V if your slave
    board doesn't already provide them.
*/

#include <Wire.h>

const uint8_t SLAVE_ADDRESS = 0x09; // I2C address of the other microcontroller
const unsigned long POLL_INTERVAL_MS = 1000;

unsigned long lastPollTime = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  } // wait for native USB serial on SAMD21

  Wire.begin(); // join I2C bus as master (no address = master mode)
  Wire.setClock(
      100000); // 100 kHz standard mode; raise to 400000 if slave supports it

  Serial.println("SAMD21 I2C Master ready.");
}

void loop() {
  if (millis() - lastPollTime >= POLL_INTERVAL_MS) {
    lastPollTime = millis();
    sendCommand('p'); // example command byte
    readResponse();
  }
}

// Send a single command byte to the slave and check for transmission errors
void sendCommand(char command) {
  Serial.print("Sending command: `");
  Serial.print(command);
  Serial.println("`...");
  Wire.beginTransmission(SLAVE_ADDRESS);
  Wire.write(command);
  Wire.flush();
  uint8_t error = Wire.endTransmission();

  switch (error) {
  case 0:
    Serial.println("Command sent successfully.");
    break;
  case 1:
    Serial.println("Error: data too long for transmit buffer.");
    break;
  case 2:
    Serial.println("Error: NACK received on address (slave not responding).");
    break;
  case 3:
    Serial.println("Error: NACK received on data.");
    break;
  case 4:
    Serial.println("Error: other I2C error.");
    break;
  default:
    Serial.println("Error: unknown I2C error.");
    break;
  }
}

// Request a response byte from the slave and verify it actually arrived
void readResponse() {
  const uint8_t expectedBytes = 1;
  uint8_t received = Wire.requestFrom(SLAVE_ADDRESS, expectedBytes);

  if (received == expectedBytes && Wire.available()) {
    char value = Wire.read();
    Serial.print("Received from slave: ");
    Serial.println(value);
  } else {
    Serial.print("Error: expected ");
    Serial.print(expectedBytes);
    Serial.print(" byte(s), got ");
    Serial.println(received);
  }
}
