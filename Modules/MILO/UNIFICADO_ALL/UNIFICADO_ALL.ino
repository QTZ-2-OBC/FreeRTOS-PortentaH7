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

  String i2c_response = "";
  Wire.requestFrom((uint8_t)OPENMV_ADDR, (uint8_t)64);
  
  while (Wire.available()) {
    char c = Wire.read();
    // OpenMV rellena con caracteres nulos o FF cuando no hay datos
    if (c != 0 && c != 255 && c != '\n') { 
      i2c_response += c;
    }
  }
  
  Serial.print("Respuesta I2C: ");
  Serial.println(i2c_response);
  Serial1.print(i2c_response);

  // Comando R para activar el Envío de la imagen por SPI 
  if(cmd == 'R' && i2c_response.indexOf("SPI_READY") >= 0) {
    
    Serial.println("Iniciando comunicacion SPI...");
    delay(100); 
    char datos_recibidos[17]; // 16 bytes + caracter nulo para imprimir
    
    // Configuración para SPI 
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));//cambio a 250000

    digitalWrite(csPin, LOW); 
    delay(5); // Pequeño delay de estabilización
    
    // Transferencia SPI bidireccional (Enviamos 16, recibimos 16)
    for (int i = 0; i < 16; i++) {
      datos_recibidos[i] = SPI.transfer(str[i]);
    }
    datos_recibidos[16] = '\0'; // Terminador nulo para evitar basura en Serial.print
    
    digitalWrite(csPin, HIGH); 
    SPI.endTransaction();
    
    Serial.print("Recibido via SPI (Tamano Imagen): ");
    Serial.println(datos_recibidos);
    
    delay(2000); // Retraso antes de la siguiente petición
  } 
  else if (cmd == 'R') {
    Serial.println("Error: OpenMV no envio SPI_READY");
  }
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
  }
  Serial.println("DONE!");
}
