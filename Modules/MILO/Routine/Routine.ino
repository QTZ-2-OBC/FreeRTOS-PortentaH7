#include "../../../Common/obc_submodules_sdk.c"
#include <ArduinoRS485.h>
#include <Wire.h>

constexpr uint32_t RS485_BAUD = 115200;
constexpr uint32_t RS485_INTERBYTE_TIMEOUT_MS = 50;

constexpr uint8_t I2C_SLAVE_ADDR = 0x42;
// constexpr uint32_t I2C_CLOCK_HZ = 115200;
constexpr uint32_t I2C_RESPONSE_TIMEOUT_MS = 100;
constexpr uint32_t I2C_POLL_INTERVAL_MS = 2;

constexpr uint8_t OPENMV_RESET_PIN = 3;

const QTZ_OBC_Packet TIMEOUT_RESPONSE = {
    .protocol_id = QTZ_OBC_PROTOCOL_SUBSYSTEMS,
    .status = QTZ_OBC_RESULT_TIMEOUT,
    .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
    .cmd_id = QTZ_OBC_COMMAND_TIMEOUT,
    .param0 = 0,
    .param1 = 0,
};

// -- BUFFERS --
uint8_t req_buffer[QTZ_OBC_PACKET_LEN];
uint8_t res_buffer[QTZ_OBC_PACKET_LEN];

QTZ_ByteArray REQ = {
    .length = 0,
    .capacity = QTZ_OBC_PACKET_LEN,
    .data = req_buffer,
};

QTZ_ByteArray RES = {
    .length = 0,
    .capacity = QTZ_OBC_PACKET_LEN,
    .data = res_buffer,
};

void setup() {
  Serial.begin(115200);

  RS485.begin(RS485_BAUD);
  RS485.receive();

  Wire.begin();
  // Wire.setClock(I2C_CLOCK_HZ);

  pinMode(OPENMV_RESET_PIN, OUTPUT);
  digitalWrite(OPENMV_RESET_PIN, 1);
}

void loop() {
  QTZ_ByteArray_Reset(&REQ);
  QTZ_ByteArray_Reset(&RES);

  if (receiveRS485Command(&REQ)) {
    bool ok = sendI2CCommandAndWait(&REQ, &RES, I2C_RESPONSE_TIMEOUT_MS);
    if (!ok) {
      Serial.println("Timeout waiting for response from I2C!");
      if (QTZ_OBC_RESULT_OK != QTZ_OBC_WritePacket(&RES, TIMEOUT_RESPONSE)) {
        Serial.println("Failed to write the timeout to the response buffer!");
      }
    } else {
      // Reset the OpenMV cam once the picture has been taken...
      QTZ_OBC_Packet p = {0};
      QTZ_OBC_ParsePacket(&RES, &p);
      if (p.cmd_id == QTZ_OBC_COMMAND_MILO_PICTURE_CLASI_ACK) {
        // Reset corto
        Serial.println("Resetting Cam...");
        Serial.println("Reset LOW");
        digitalWrite(OPENMV_RESET_PIN, LOW);
        delay(500);

        Serial.println("Reset HIGH");
        digitalWrite(OPENMV_RESET_PIN, HIGH);
      }
    }

    sendRS485Response(&RES);
  }
}

// Blocks until a full buffer has been read from RS485.
bool receiveRS485Command(QTZ_ByteArray *buff) {
  uint8_t received = 0;
  uint32_t last_byte_at = millis();

  while (received < buff->capacity) {
    if (RS485.available()) {
      int b = RS485.read();
      if (b < 0) {
        continue;
      }
      buff->length += 1;
      QTZ_ByteArray_Set(buff, received, b);
      received++;
      last_byte_at = millis();
    } else if (received > 0 &&
               (millis() - last_byte_at > RS485_INTERBYTE_TIMEOUT_MS)) {
      // Frame stalled mid-way, discard and start over.
      received = 0;
      QTZ_ByteArray_Reset(buff);
    }
  }

  return true;
}

bool sendI2CCommandAndWait(QTZ_ByteArray *req_buff, QTZ_ByteArray *res_buff,
                           uint32_t timeout_ms) {
  Wire.beginTransmission(I2C_SLAVE_ADDR);
  Wire.write(req_buff->data, req_buff->length);
  if (Wire.endTransmission() != 0) {
    return false; // NACK / bus error sending command.
  }

  uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    uint8_t n = Wire.requestFrom((int)I2C_SLAVE_ADDR, res_buff->capacity);
    if (n == res_buff->capacity) {
      res_buff->length = res_buff->capacity;
      for (uint8_t i = 0; i < res_buff->capacity; i++) {
        QTZ_ByteArray_Set(res_buff, i, Wire.read());
      }
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

void sendRS485Response(QTZ_ByteArray *buff) {
  RS485.beginTransmission();
  RS485.write(buff->data, buff->length);
  RS485.endTransmission();
}
