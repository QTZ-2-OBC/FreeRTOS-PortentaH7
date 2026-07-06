#include <SPI.h>
#include <stdlib.h>

#define SS_PIN 7

const int OPENMV_RESET_PIN = 2;
const bool MODEL_ENABLED = false;

// ------------ Listado de comandos ------------ //
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
    "P", "[P]ing",

    "R", "[R]ecive image",
};

char input = 0;
int weight_images = 0;

// ------------ Función para imprimir menú de comandos ------------ //
void printMenu() {
  Serial.println("====== MENU ======");
  for (size_t i = 0; i < COMMAND_QUANTITY * 2; i += 2) {
    Serial.print(COMMANDS[i]);
    Serial.print(" - ");
    Serial.println(COMMANDS[i + 1]);
  }
  Serial.println("==================");
}

// ------------ Función para comunicación SPI ------------ //
void sendCommand(char cmd) {
  // Transacción 1: enviar comando
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE2));
  digitalWrite(SS_PIN, LOW);
  delayMicroseconds(100);
  SPI.transfer(cmd);
  delayMicroseconds(50);
  digitalWrite(SS_PIN, HIGH);
  SPI.endTransaction();

  delay(getDelay(cmd));   //El tiempo cambia respecto al comando

  // Leer respuesta byte a byte
  char buf[32];
  memset(buf, 0, sizeof(buf));  // ← inicializar a cero

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE2));
  digitalWrite(SS_PIN, LOW);
  delayMicroseconds(100);

  for (int i = 0; i < 31; i++) {
    buf[i] = SPI.transfer(0x00);
    if (buf[i] == '\n') { buf[i] = 0; break; }
  }

  delayMicroseconds(100);
  digitalWrite(SS_PIN, HIGH);
  SPI.endTransaction();

  Serial.print("Respuesta: ");
  Serial.println(buf);
}

// ------------ Función para comunicación SPI: Caso especial comando 'R' ------------ //
void sendCommand_CasoR(char cmd) {
  // Transacción 1: enviar comando
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE2));
  digitalWrite(SS_PIN, LOW);
  delayMicroseconds(100);
  SPI.transfer(cmd);
  delayMicroseconds(50);
  digitalWrite(SS_PIN, HIGH);
  SPI.endTransaction();

  delay(getDelay(cmd));   //El tiempo cambia respecto al comando

  // Leer respuesta byte a byte
  char buf[32];
  memset(buf, 0, sizeof(buf));  // ← inicializar a cero

  //Transacción 2: recibir tamaño de la imagen

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE2));
  digitalWrite(SS_PIN, LOW);
  delayMicroseconds(100);

  for (int i = 0; i < 31; i++) {
    buf[i] = SPI.transfer(0x00);
    if (buf[i] == '\n') { buf[i] = 0; break; }
  }

  delayMicroseconds(100);
  digitalWrite(SS_PIN, HIGH);
  SPI.endTransaction();

  weight_images = atoi(buf);
  Serial.print("El tamaño de la imagen a recibir es:");
  Serial.println(weight_images);

  delay(200);

  //Transacción 3: recibir datos de imagenes por chunks

  const int chunk = 64;
  uint8_t buf_2[chunk];

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE2));
  digitalWrite(SS_PIN, LOW);
  delayMicroseconds(100);

  int datos_restantes = weight_images;
  while (datos_restantes > 0){
    int read = min(datos_restantes, chunk);
    for (int i = 0; i < read; i++) {
      buf[i] = SPI.transfer(0x00);
    }
    Serial.write(buf, read);
    datos_restantes -= read;
    delay(15);
  }

  digitalWrite(SS_PIN, HIGH);
  SPI.endTransaction();

  Serial.println("\nImagen recibida.");
}

// ------------ Función para obtener el delay dependiendo del comando ------------ //
int getDelay(char cmd) {
  switch(cmd) {
    case 'S': return 800;   // snapshot: guardar JPEG es lento
    case 'E':
    case 'H':
    case 'T': return 6000;  // cargar modelo
    case 'O': return 200;   // gc.collect
    default:  return 150;   // comandos rápidos
  }
}

// ------------ Set-up de inicialización ------------ //
void setup() {
  Serial.begin(115200);

  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);

  SPI.begin();

  pinMode(OPENMV_RESET_PIN, OUTPUT);
  digitalWrite(OPENMV_RESET_PIN, 1);

  delay(2000);
  printMenu();
}

void loop() {
  if (MODEL_ENABLED) {
    sendCommand('q'); // consulta modelo
    delay(500);
  }

  if (Serial.available()) {
    input = Serial.read();
    if (input == '\n') return;
    Serial.print("Received command: ");
    Serial.println(input);
  }

  // Search through available commands...
  int idx = -1;
  for (int i = 0; i < COMMAND_QUANTITY * 2; i += 2) {
    const char cmd = COMMANDS[i][0];
    if (input == cmd) {
      idx = i;
      break;
    }
  }

  if (idx != -1) { // Command is not found (invalid)!
    if (input == 'r'){
          // Reset corto
    Serial.println("Resetting Cam...");
    Serial.println("Reset LOW");
    digitalWrite(OPENMV_RESET_PIN, LOW);
    delay(1000);

    Serial.println("Reset HIGH");
    digitalWrite(OPENMV_RESET_PIN, HIGH);
    }

    if (input == 'R') sendCommand_CasoR(input);

    else sendCommand(input);

    printMenu();
  }
}