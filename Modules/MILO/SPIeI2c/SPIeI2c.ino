#include "SPI.h"

char str[] = "Estoy vivo \n";
const int csPin = 2;

void setup() {
  Serial.begin(115200); // establece la velocidad de transmisión a 115200 para USART
  digitalWrite(csPin, HIGH); 

  //Inicialización de comunicación SPI
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8); // divide el reloj por 8
  Serial.println("Hola, soy SPI Mega_Master");
}

void loop(void) {
  char dat_reciv[16]; 
  digitalWrite(csPin, LOW); // habilitar Selección de esclavo
  // enviar cadena de prueba
  for (int i = 0; i < sizeof(str); i++)
    SPI.transfer(str[i]);
  digitalWrite(csPin, HIGH); // deshabilitar la selección de esclavo
  delay(2000);
  dat_esclavo = SPI.transfer(esclavo1); 
  if(dat_esclavo)
  {
    Serial.println("dat_esclavo"); 
  }
  else
  {
    Serial.println("No hay dato recibido"); 
  }
  delay(1000); 
}