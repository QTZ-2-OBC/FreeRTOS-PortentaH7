#include <Wire.h>
#include "SPI.h"

const uint8_t OPENMV_ADDR = 0x42;
const uint8_t SAMD_ADDR = 0x41;
const uint8_t PORTENTA_ADDR = 0x40;

const int OPENMV_RESET_PIN = 3;
const bool MODEL_ENABLED = false;
const size_t MAX_LOOP_ITERS = 25000;

char str[16] = "Estoy vivo\n"; //Prueba de cadena de envío
const int csPin = 2; //Pin selector de esclavo.

const int TX_ENABLE_PIN = 4;  // Connects to DE and RE of the transceiver

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200); // Pins 13 y 14
  Wire.begin();

  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);

  //Configuración de comunicación SPI
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8);
  Serial.println("Hola, soy SPI Mega_Master");

  pinMode(OPENMV_RESET_PIN, OUTPUT);
  digitalWrite(OPENMV_RESET_PIN, 1);

  pinMode(TX_ENABLE_PIN, OUTPUT);
  digitalWrite(TX_ENABLE_PIN, LOW); // Start in Receive Mode

  delay(2000);
  printMenu();
}

const size_t COMMAND_QUANTITY = 14;
const char *COMMANDS[] = {
    "S", "[S]napshot",

    "E", "MODEL [E]ARTHLIMB ON",
    "H", "MODEL [H]YPSO ON",
    "T", "MODEL SEN[T]INEL ON",

    "O", "Turn [o]ff Model",

    "B", "[B]rightness +",
    "b", "[B]rightness -",
    "C", "[C]ontrast +",
    "c", "[C]ontrast -",
    "s", "[S]tatus",
    "r", "[R]eset OpenMV Cam",
    "i", "[I]mage Result",
    "p", "[P]ing",
    "R", "Cambio a SPI",
};

void printMenu() {
  Serial.println("====== MENU ======");
  for (size_t i = 0; i < COMMAND_QUANTITY * 2; i += 2) {
    Serial.print(COMMANDS[i]);
    Serial.print(" - ");
    Serial.println(COMMANDS[i + 1]);
  }
  Serial.println("==================");
}

void sendCommand(char cmd) {
  Wire.beginTransmission(OPENMV_ADDR);
  Wire.write(cmd);
  Wire.endTransmission();
  delay(100);

  char i2c_response[64] = {};
  Wire.requestFrom((uint8_t)OPENMV_ADDR, (uint8_t)64);

  for (int i = 0; i < 64; i++) {
    if (Wire.available()) {
      char c = Wire.read();
      if (c != 0 && c != 255 && c != '\n') {
        i2c_response[i] = c;
      }
    }
  }

  Serial.print("Respuesta I2C: '");
  Serial.print(i2c_response);
  Serial.println("'");

  digitalWrite(TX_ENABLE_PIN, HIGH);
  Serial1.print (i2c_response);
  Serial1.flush();
  digitalWrite(TX_ENABLE_PIN, LOW);

  /*
  //---------------------------------------------------------------------
  //------------------Logica para transmisión de Imagen -----------------
  //---------------------------------------------------------------------

    if (cmd == 'S' && strncmp(i2c_response, "SIZE:", 5) == 0) {
      int total_size = 0;
      int total_pkts = 0;

      // Extraemos los valores de la respuesta
      sscanf(i2c_response, "SIZE:%d,PKTS:%d", &total_size, &total_pkts);

      Serial.print("Iniciando descarga SPI. Paquetes a leer: ");
      Serial.println(total_pkts);

      SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

      uint8_t buffer_spi[1024];
      int bytes_recibidos = 0;

      // Le damos a la OpenMV un momento para preparar cada paquete
      delay(50);

      for (int p = 0; p < total_pkts; p++) {
        digitalWrite(csPin, LOW);

        for (int i = 0; i < 1024; i++) {
          buffer_spi[i] = SPI.transfer(0x00);
        }

        digitalWrite(csPin, HIGH);

        // --- GUARDAS EL PAQUETE ---

        Serial.print("Paquete ");

        // Esto solo para ver que los datos recibidos no sean basura y poder depurar
        for (int i = 0; i < 1024; i++){
          Serial.print(buffer_spi[i]);
          Serial.print(" ");
        }
        //-------------------------------------------------------------------


        Serial.println();
        Serial.print(p + 1);
        Serial.println(" recibido.");

        // Tiempo para preparar el siguiente paquete
        delay(50);
      }

      SPI.endTransaction();
      Serial.println("Imagen descargada completamente.");
    }
    */
}

void loop() {
  // printMenu();
  if (MODEL_ENABLED) {
    sendCommand('q'); // consulta modelo
    delay(500);
  }

  if (!Serial1.available()) {
    return;
  }
  char input = Serial1.read();

  Serial.print("Received command: ");
  Serial.println(input);

  // Search through available commands...
  int idx = -1;
  for (int i = 0; i < COMMAND_QUANTITY * 2; i += 2) {
    const char cmd = COMMANDS[i][0];
    if (input == cmd) {
      idx = i;
      break;
    }
  }

  if (idx == -1) { // Command is not found (invalid)!
    Serial.println("Invalid command!");
    return;
  }

  sendCommand(input);
  if (input == 'r') {
    // Reset corto
    Serial.println("Resetting Cam...");
    Serial.println("Reset LOW");
    digitalWrite(OPENMV_RESET_PIN, LOW);
    delay(1000);

    Serial.println("Reset HIGH");
    digitalWrite(OPENMV_RESET_PIN, HIGH);
    Serial1.print("0DONE");
  }
  Serial.println("DONE!");
}
