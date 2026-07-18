//---------------------------------------------------------
// TRIAD ORIENTATION ALGORITHM FOR ADCS SUSBSYSTEM
//---------------------------------------------------------

// -------- LIBRARIES AND INITIAL VALUES ------------------

// #include "BMI088.h"
// #include <7Semi_MMC5983MA.h>
#include <ArduinoRS485.h>
#include <Wire.h>
#include <math.h>

const int MAX_RETRIES = 3;
const int ACK_TIMEOUT = 50;

const int TX_ENABLE_PIN = 7; // Connects to DE and RE of the transceiver
RS485Class rs485(Serial1, TX_ENABLE_PIN, TX_ENABLE_PIN, -1);

constexpr auto baudrate{115200};

// Calculate preDelay and postDelay in microseconds for stable RS-485
// transmission
constexpr auto bitduration{1.f / baudrate};
constexpr auto wordlen{9.6f}; // OR 10.0f depending on the channel configuration
constexpr auto preDelayBR{bitduration * wordlen * 3.5f * 1e6};
constexpr auto postDelayBR{bitduration * wordlen * 3.5f * 1e6};

// MMC5983MA_7Semi mag;

float
    magVec[3]; // Campo magnetico en el MARCO DEL CUERPO (tras calibrar y rotar)
float accelVec[3];
float gyroVec[3];

float ax, ay, az;
float gx, gy, gz;
float x, y, z;

// NOTE: Always keep in sync!
const size_t COMMAND_QUANTITY = 5;
const char *COMMANDS[] = {
    "p", "[P]ing",     // Ping the microcontroller to know if it's still alive.
    "a", "[A]wake",    // Wake up the microcontroller from a low power mode.
    "s", "[S]leep",    // Make the microcontroller enter a low power mode.
    "g", "[G]et Data", // Get sensor data from the microcontroller.
    "r", "[R]eset"     // Rese the microcontroller.
};

/* accel object */
// Bmi088Accel accel(Wire, 0x18);
/* gyro object */
// Bmi088Gyro gyro(Wire, 0x68);

// Vectores de REFERENCIA (PROVISIONALES - reemplazar por IGRF y efemerides del
// Sol). Deben estar en el MISMO marco (p.ej. ECI) y se normalizan dentro de
// triad().
float sunRef[3] = {1, 0, 0}; // direccion del Sol en referencia (placeholder)
float magRef[3] = {0, 0, 1}; // direccion del campo en referencia (placeholder)

float A[3][3]; // matriz de actitud MARCO DE REFERENCIA CUERPO

// -------------------- FOTODIODOS -------------------------
// Orden de caras:  0:+X  1:-X  2:+Y  3:-Y  4:+Z  5:-Z
float pdRaw[6];        // lecturas crudas del ADC
float sunVec[3];       // vector Sol en el MARCO DEL CUERPO
bool sunValid = false; // false en eclipse o si ninguna cara ve el Sol

// Constantes de calibracion (RELLENAR tras calibrar):
// pd_offset[i] = lectura en oscuridad
// pd_gain[i]   = 1 / (pico_i - pd_offset[i])  - normaliza el pico a 1.0
float pd_offset[6] = {0, 0, 0, 0, 0, 0};
float pd_gain[6] = {1, 1, 1, 1, 1, 1};

// Umbral minimo de iluminacion para dar por valido el vector Sol
const float SUN_MIN = 0.05; // ajustalo tras ver tus magnitudes reales

// -------------------------- HELPERS ---------------------------------
// Normaliza un vector 3D (lo vuelve unitario). Devuelve la magnitud original.
float normalize3(float v[3]) {
  float n = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (n > 1e-9) {
    v[0] /= n;
    v[1] /= n;
    v[2] /= n;
  }
  return n;
}

// Producto cruz: out = a x b
void cross3(const float a[3], const float b[3], float out[3]) {
  out[0] = a[1] * b[2] - a[2] * b[1];
  out[1] = a[2] * b[0] - a[0] * b[2];
  out[2] = a[0] * b[1] - a[1] * b[0];
}

// -------- MAGNETOMETER FRAME TRANSFORM ------------------
// Rotacion FIJA del marco del magnetometro al marco del cuerpo.
// (montaje girado 180 grados sobre X => invierte Y y Z, det = +1):
const float R_mag[3][3] = {{1, 0, 0}, {0, -1, 0}, {0, 0, -1}};

// Offsets del magnetometro EN EL MARCO DEL SENSOR (de la calibracion en MATLAB)
// const float mag_offset[3] = { 107.89-98.05, 9.62-299.2, -269.29-3494.79};
const float mag_offset[3] = {120.98, 290.84, -3794.84};

// Aplica una rotacion 3x3 a un vector: v_out = R * v_in
void applyRotation(const float R[3][3], const float v_in[3], float v_out[3]) {
  for (int i = 0; i < 3; i++) {
    v_out[i] = 0;
    for (int j = 0; j < 3; j++)
      v_out[i] += R[i][j] * v_in[j];
  }
}

void printMenu() {
  Serial.println("====== MENU ======");
  for (size_t i = 0; i < COMMAND_QUANTITY * 2; i += 2) {
    Serial.print(COMMANDS[i]);
    Serial.print(" - ");
    Serial.println(COMMANDS[i + 1]);
  }
  Serial.println("==================");
}

void sendRS485(char *msg) {
  Serial.print("Response: ");
  Serial.println(msg);

  int acknowledged = 0;
  for (int i = 0; i < MAX_RETRIES; i++) {
    rs485.beginTransmission();
    rs485.print(msg);
    rs485.endTransmission();

    unsigned long target = millis() + ACK_TIMEOUT;
    while (millis() <= target) {
      if (rs485.available()) {
        char ack = rs485.read();
        Serial.print("Received ACK: ");
        Serial.println(ack);

        if (ack == 'K') {
          acknowledged = 1;
          break;
        }
      }
    }

    if (acknowledged) {
      break;
    }
    Serial.println("Didn't receive any acknowledgement from PortentaH7! Retrying...");
  }

  if (!acknowledged) {
    Serial.println("No response received! Failed to send respone...");
  }
}

// ----- I2C COMUNICATION SETUP AND SENSOR INITIALIZING -----
void setup() {
  Serial.begin(9600); // Serial data BaudRate
  delay(100);

  rs485.begin(baudrate);
  rs485.setDelays(preDelayBR, postDelayBR);
  rs485.receive();

  int status;
  // USB Serial to print data. Bounded wait: if no USB host is attached
  // (e.g. running standalone on battery/carrier power), Serial never
  // becomes truthy and this used to hang forever before the board could
  // ever reach loop() and service RS485 commands.
  unsigned long serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 3000) {
  }
  /* start the sensors */
  // status = accel.begin();
  // if (status < 0) {
  //   Serial.println("Accel Initialization Error");
  //   Serial.println(status);
  //   while (1) {
  //   }
  // }
  // status = gyro.begin();
  // if (status < 0) {
  //   Serial.println("Gyro Initialization Error");
  //   Serial.println(status);
  //   while (1) {
  //   }
  // }

  // if (!mag.beginI2C(Wire, 0x30)) {
  //   Serial.println("MMC5983MA not found on I2C");
  //   while (1)
  //     delay(10);
  // }

  // mag.enableAutoSetReset(true, MMC5983MA_7Semi::MES_100);

  delay(2000);
  printMenu();

  // Serial.println("MMC5983MA OK");
}

// -------------------------- MAIN LOOP ----------------------------
void loop() {
  if (!rs485.available()) {
    return;
  }
  char input = rs485.read();
  Serial.print("Received: ");
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

  switch (input) {
  case 'p': {
    sendRS485("0PING");
  } break;
  case 'a': {
    // TODO: Wake up module...
    sendRS485("0AWAKE");
  } break;
  case 's': {
    // TODO: Sleep module...
    sendRS485("0SLEEP");
  } break;
  case 'g': {
    // Tras esta llamada, magVec queda en el MARCO DEL CUERPO, listo para TRIAD
    readmag(magVec[0], magVec[1], magVec[2], 2);
    readIMU();
    sunValid = computeSunVector(sunVec);

    // TODO: Send bytes read from sensor...
    sendRS485("0BYTES");
  } break;
  case 'r': {
    // TODO: Reset microcontroller...
    sendRS485("0DONE");
  } break;
  }

  // if (sunValid) {
  // // Ancla = Sol; segundo vector = campo magnetico
  // triad(sunVec, magVec, sunRef, magRef, A);
  // printAttitude(A);
  // } else {
  // Serial.println("Eclipse: sin vector Sol, TRIAD en pausa");
  // }
  // delay(1000);  //  mas rapido para capturar mas puntos
}

// ---------------------- LECTURA DE LA IMU -----------------------------
void readIMU() {

  // Initial reading on both sensors
  // accel.readSensor();
  // gyro.readSensor();

  // // accelVec[0] = accel.getAccelX_mss();
  // // accelVec[1] = accel.getAccelY_mss();
  // // accelVec[2] = accel.getAccelZ_mss();

  // gyroVec[0] = gyro.getGyroX_rads();
  // gyroVec[1] = gyro.getGyroY_rads();
  // gyroVec[2] = gyro.getGyroZ_rads();

  // print the data (CORREGIDO: antes leia el indice [3], fuera de rango)
  Serial.print("Acc: ");
  Serial.print(accelVec[0]);
  Serial.print("\t");
  Serial.print(accelVec[1]);
  Serial.print("\t");
  Serial.print(accelVec[2]);
  Serial.print("\t");
  Serial.print("Gyro: ");
  Serial.print(gyroVec[0]);
  Serial.print("\t");
  Serial.print(gyroVec[1]);
  Serial.print("\t");
  Serial.print(gyroVec[2]);
  Serial.print("\t");
  // Serial.print(accel.getTemperature_C());
  Serial.print("\n");
}

// --------------------- LECTURA DEL MAGNETOMETRO ----------------------
void readmag(float &x_cal, float &y_cal, float &z_cal, int mode) {
  // switch (mode) {
  // case 1:
  //   if (mag.readMagnetometer(x, y, z)) {

  //     /**  MAGNETOMETER CALIBRATION MODE (raw para MATLAB) */
  //     Serial.print(x);
  //     Serial.print(",");
  //     Serial.print(y);
  //     Serial.print(",");
  //     Serial.println(z);

  //   } else {
  //     Serial.println("Mag read failed");
  //   }

  //   break;

  // case 2:
  //   if (mag.readMagnetometer(x, y, z)) {

  //     /**  CALIBRATED MODE + ROTACION A MARCO DEL CUERPO **/

  //     // 1) Calibracion: restar offsets EN EL MARCO DEL SENSOR
  //     float mag_sensor[3];
  //     mag_sensor[0] = x - mag_offset[0];
  //     mag_sensor[1] = y - mag_offset[1];
  //     mag_sensor[2] = z - mag_offset[2];

  //     // 2) Rotar del marco SENSOR al marco del CUERPO
  //     float mag_body[3];
  //     applyRotation(R_mag, mag_sensor, mag_body);

  //     // 3) Escribir el resultado (marco cuerpo) en las referencias = magVec
  //     x_cal = mag_body[0];
  //     y_cal = mag_body[1];
  //     z_cal = mag_body[2];

  //     // |B| es invariante ante la rotacion
  //     float B_total = sqrt(x_cal * x_cal + y_cal * y_cal + z_cal * z_cal);

  //     Serial.print("Body mG: ");
  //     Serial.print(x_cal, 2);
  //     Serial.print(", ");
  //     Serial.print(y_cal, 2);
  //     Serial.print(", ");
  //     Serial.print(z_cal, 2);
  //     Serial.print(" | |B|: ");
  //     Serial.print(B_total, 2);
  //     Serial.println(" mG");

  //   } else {
  //     Serial.println("Mag read failed");
  //   }

  //   break;

  // default:
  //   break;
  // }
}

// ---------------------- LECTURA DE FOTODIODOS ---------------------------
// Valores inventados en pines.
void readPhotodiodesRaw() {
  // const int pdPin[6] = {A0, A1, A2, A3, A6, A7}; // Cambiar esto
  // for (int i = 0; i < 6; i++)
  //   pdRaw[i] = analogRead(pdPin[i]);
}

// para calibrar offsets y picos
void printPhotodiodesRaw() {
  readPhotodiodesRaw();
  for (int i = 0; i < 6; i++) {
    Serial.print(pdRaw[i]);
    Serial.print(i < 5 ? "," : "\n");
  }
}

// Lectura calibrada (~cos del angulo) de un fotodiodo, en escala comun
float readDiodeCalibrated(int i) {
  float val = (pdRaw[i] - pd_offset[i]) * pd_gain[i];
  if (val < 0)
    val = 0; // cara en sombra -> 0
  return val;
}

// Construye el vector Sol en MARCO DEL CUERPO. false si no hay Sol (eclipse).
bool computeSunVector(float out_body[3]) {
  readPhotodiodesRaw();

  float I[6];
  for (int i = 0; i < 6; i++)
    I[i] = readDiodeCalibrated(i);

  // Diferencia entre caras opuestas -> componente del vector Sol
  out_body[0] = I[0] - I[1]; // +X  -  -X
  out_body[1] = I[2] - I[3]; // +Y  -  -Y
  out_body[2] = I[4] - I[5]; // +Z  -  -Z

  float mag = normalize3(out_body);
  return (mag > SUN_MIN); // valido solo si hay iluminacion suficiente
}

// ------------------------------- TRIAD --------------------------------------
//   b1,b2 = vectores medidos en el CUERPO
//   r1,r2 = los mismos vectores conocidos en REFERENCIA
//   b1/r1 = vector ANCLA (el mas confiable; aqui el Sol)
//   A     = matriz de rotacion resultante = la orientacion del satelite
void triad(const float b1[3], const float b2[3], const float r1[3],
           const float r2[3], float A[3][3]) {

  // Copias normalizadas (no destruimos los originales)
  float u1[3] = {b1[0], b1[1], b1[2]};
  normalize3(u1);
  float w2[3] = {b2[0], b2[1], b2[2]};
  normalize3(w2);
  float s1[3] = {r1[0], r1[1], r1[2]};
  normalize3(s1);
  float s2[3] = {r2[0], r2[1], r2[2]};
  normalize3(s2);

  // Triada en el CUERPO
  float t1b[3] = {u1[0], u1[1], u1[2]};
  float t2b[3];
  cross3(u1, w2, t2b);
  normalize3(t2b);
  float t3b[3];
  cross3(t1b, t2b, t3b);

  // Triada en REFERENCIA
  float t1r[3] = {s1[0], s1[1], s1[2]};
  float t2r[3];
  cross3(s1, s2, t2r);
  normalize3(t2r);
  float t3r[3];
  cross3(t1r, t2r, t3r);

  // A = [t1b t2b t3b] * [t1r t2r t3r]^T
  float Mb[3][3] = {{t1b[0], t2b[0], t3b[0]},
                    {t1b[1], t2b[1], t3b[1]},
                    {t1b[2], t2b[2], t3b[2]}};
  float Mr[3][3] = {{t1r[0], t2r[0], t3r[0]},
                    {t1r[1], t2r[1], t3r[1]},
                    {t1r[2], t2r[2], t3r[2]}};

  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) {
      A[i][j] = 0;
      for (int k = 0; k < 3; k++)
        A[i][j] += Mb[i][k] * Mr[j][k];
    }
}

// Imprime la actitud como Euler ZYX (yaw, pitch, roll) en grados. Solo para
// display.
void printAttitude(const float A[3][3]) {
  float yaw = atan2(A[1][0], A[0][0]) * 180.0 / PI;
  float pitch = asin(-A[2][0]) * 180.0 / PI;
  float roll = atan2(A[2][1], A[2][2]) * 180.0 / PI;

  Serial.print("Yaw: ");
  Serial.print(yaw, 1);
  Serial.print("  Pitch: ");
  Serial.print(pitch, 1);
  Serial.print("  Roll: ");
  Serial.println(roll, 1);
}
