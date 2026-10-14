#include <ArduinoRS485.h>
#include <Wire.h>

constexpr uint32_t RS485_BAUD = 115200;
constexpr uint32_t RS485_INTERBYTE_TIMEOUT_MS = 50;

constexpr uint8_t I2C_SLAVE_ADDR = 0x42;
// constexpr uint32_t I2C_CLOCK_HZ = 115200;
constexpr uint32_t I2C_RESPONSE_TIMEOUT_MS = 100;
constexpr uint32_t I2C_POLL_INTERVAL_MS = 2;

constexpr uint8_t OPENMV_RESET_PIN = 3;

const uint8_t TIMEOUT_RESPONSE[] = {'S', 2, 'P', 20, 0, 0};

// -- BUFFERS --
#define QTZ_OBC_PACKET_LEN 6
uint8_t req_buffer[QTZ_OBC_PACKET_LEN];
uint8_t res_buffer[QTZ_OBC_PACKET_LEN];

void setup() {
  Serial.begin(115200);

  RS485.begin(RS485_BAUD);
  RS485.receive();

  Wire.begin();
  // Wire.setClock(I2C_CLOCK_HZ);

  pinMode(OPENMV_RESET_PIN, OUTPUT);
  digitalWrite(OPENMV_RESET_PIN, 1);
}

void print_command(uint8_t* buff) {
  for (int i = 0; i < QTZ_OBC_PACKET_LEN; i++) {
    Serial.print(buff[i]);
  }
  Serial.println();
}

void loop() {
  if (receiveRS485Command()) {
    Serial.println("Command received!");
    print_command(req_buffer);

    bool ok = sendI2CCommandAndWait(I2C_RESPONSE_TIMEOUT_MS);
    if (!ok) {
      Serial.println("Timeout waiting for response from I2C!");
      // if (QTZ_OBC_RESULT_OK != QTZ_OBC_WritePacket(&RES, TIMEOUT_RESPONSE)) {
      // Serial.println("Failed to write the timeout to the response buffer!");
      // }
    } else {
      // Reset the OpenMV cam once the picture has been taken...
      if (res_buffer[3] == 19) {
        // Reset corto
        Serial.println("Resetting Cam...");
        Serial.println("Reset LOW");
        digitalWrite(OPENMV_RESET_PIN, LOW);
        delay(500);

        Serial.println("Reset HIGH");
        digitalWrite(OPENMV_RESET_PIN, HIGH);
      }
    }

    sendRS485Response();
  }
}

// Blocks until a full buffer has been read from RS485.
bool receiveRS485Command() {
  uint8_t received = 0;
  uint32_t last_byte_at = millis();

  while (received < QTZ_OBC_PACKET_LEN) {
    if (RS485.available()) {
      int b = RS485.read();
      if (b < 0) {
        continue;
      }
      req_buffer[received] = b;
      received++;
      last_byte_at = millis();
    } else if (received > 0 &&
               (millis() - last_byte_at > RS485_INTERBYTE_TIMEOUT_MS)) {
      // Frame stalled mid-way, discard and start over.
      received = 0;
    }
  }

  return true;
}

bool sendI2CCommandAndWait(uint32_t timeout_ms) {
  Wire.beginTransmission(I2C_SLAVE_ADDR);
  Wire.write(req_buffer, QTZ_OBC_PACKET_LEN);
  if (Wire.endTransmission() != 0) {
    return false; // NACK / bus error sending command.
  }

  uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    uint8_t n = Wire.requestFrom((int)I2C_SLAVE_ADDR, QTZ_OBC_PACKET_LEN);
    if (n == QTZ_OBC_PACKET_LEN) {
      for (uint8_t i = 0; i < QTZ_OBC_PACKET_LEN; i++) {
        res_buffer[i] = Wire.read();
      }
      Serial.println("Received response!");
      print_command(res_buffer);
      return true;
    }
    while (Wire.available()) {
      Wire.read(); // Drain any garbage bytes
    }
    delay(I2C_POLL_INTERVAL_MS);
  }

  // Timed out
  return false;
}

void sendRS485Response() {
  RS485.beginTransmission();
  RS485.write(res_buffer, QTZ_OBC_PACKET_LEN);
  RS485.endTransmission();
}
