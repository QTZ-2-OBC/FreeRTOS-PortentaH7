#include "SPI.h"


char str[16] = "Estoy vivo\n"; //Prueba de cadena de envío 
const int csPin = 2; //Pin selector de esclavo. 

void setup() {
  Serial.begin(115200); 
  
  //Configuración de pin selector. 
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);
  
  //Configuración de comunicación SPI 
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8); 
  Serial.println("Hola, soy SPI Mega_Master");
}

void loop(void) {
  char datos_recibidos[16]; // Buffer para guardar lo que envía el esclavo
  digitalWrite(csPin, LOW); // Habilitar Selección de esclavo
  delay(10); 
  
  //Envío de cadena para el esclavo
  for (int i = 0; i < 16; i++) {

    datos_recibidos[i] = SPI.transfer(str[i]);
  }
  
  digitalWrite(csPin, HIGH); // Deshabilitar el selector de esclavo 

  // Leer y mostrar lo recibido por el esclavo
  Serial.print("Recibido del esclavo: ");
  for(int i = 0; i < 16; i++){
    Serial.print(datos_recibidos[i]);
  }
  Serial.println();
  delay(2000);//Retraso para iniciar nuevo buble. 
}