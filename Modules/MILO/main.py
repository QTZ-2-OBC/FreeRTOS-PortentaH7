import sensor
import time
import ml
import uos
import gc
from pyb import I2C
from pyb import SPI, Pin

OK_STATUS = "0"

# Sensor
def init_sensor():
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.set_windowing((240, 240))
    sensor.skip_frames(time=2000)


init_sensor()

spi = SPI(2, SPI.SLAVE, baudrate=1000000, polarity=0, phase=0)
print("Esperando datos SPI...")

def minimize_label(label: str) -> str:
    if label == "Cloudy_Medium":
        return "M"
    elif label == "Cloudy_High":
        return "H"
    elif label == "Cloudy_Low":
        return "L"
    else:
        return "N"


# Storage
try:
    uos.mkdir("images")
except:
    pass

# I2C
i2c = I2C(2, I2C.SLAVE, addr=0x42)

# Variables
net = None
current_model = 0
mode = 0

contrast_val = 0
brightness_val = 0
MIN_VAL = -5
MAX_VAL = 5

last_detection_time = 0
cooldown_ms = 5000

best_label = "None"
best_score = 0.0
detected_flag = 0
saved_flag = 0

#variables para SPI
receive_buffer = bytearray(1024)
send_buffer = bytearray(1024)

clock = time.clock()


# ---------------- LABELS (por modelo) ----------------
def load_labels(path):
    return [line.rstrip('\n') for line in open(path)]


# ---------------- LOAD MODEL ----------------
def load_model(model_id):
    global net, current_model, labels

    print("Cambiando modelo...")

    if net is not None:
        net = None
        gc.collect()

    if model_id == 1:
        net = ml.Model("Milo/EarthLimb/trained.tflite")
        labels = load_labels("Milo/EarthLimb/labels.txt")
        print("EarthLimb cargado")

    elif model_id == 2:
        net = ml.Model("Milo/Hypso/trained.tflite")
        labels = load_labels("Milo/Hypso/labels.txt")
        print("Hypso cargado")

    elif model_id == 3:
        net = ml.Model("Milo/Sentinel/trained.tflite")
        labels = load_labels("Milo/Sentinel/labels.txt")
        print("Sentinel cargado")

    current_model = model_id
    gc.collect()

# ---------------- LOOP ----------------
while True:

    saved_flag = 0

    # -------- INFERENCIA --------
    if mode in [1, 2, 3] and net is not None:

        clock.tick()
        img = sensor.snapshot()

        try:
            predictions = net.predict([img])[0].flatten().tolist()
            best_label, best_score = max(zip(labels, predictions), key=lambda x: x[1])

        except Exception as e:
            print("Error inferencia:", e)
            best_label = "Error"
            best_score = 0

        if best_label == "Cloudy_Medium" and best_score > 0.6:

            detected_flag = 1
            current_time = time.ticks_ms()

            if time.ticks_diff(current_time, last_detection_time) > cooldown_ms:

                filename = "images/cloudy_%d.jpg" % current_time
                img.save(filename, quality=85)
                gc.collect()

                last_detection_time = current_time
                saved_flag = 1
        else:
            detected_flag = 0

    # -------- I2C --------
    try:
        data = i2c.recv(1, timeout=10)

        if data:

            cmd = data.decode()
            response = ""
            response_status = OK_STATUS

            # -------- MODELOS --------
            if cmd == 'E':
                load_model(1)
                mode = 1
                response = "MODEL EARTHLIMB ON"

            elif cmd == 'H':
                load_model(2)
                mode = 2
                response = "MODEL HYPSO ON"

            elif cmd == 'T':
                load_model(3)
                mode = 3
                response = "MODEL SENTINEL ON"

            elif cmd == 'O':
                mode = 0
                net = None
                gc.collect()
                response = "MODELS OFF"

            # -------- CAPTURA --------
            elif cmd == 'S':
                # 1. Tomar y comprimir la imagen
                img = sensor.snapshot()
                img_jpeg = img.compress(quality=50)
                datos_jpeg = img_jpeg.bytearray()

                total_size = len(datos_jpeg)
                paquete_size = 1024
                # Calculo del total de paquetes
                total_pkts = (total_size + paquete_size - 1) // paquete_size

                print("Tamaño JPEG:", total_size, "bytes en", total_pkts, "paquetes")

                # Enviar la cantidad de paquetes que se van a enviar
                response = "SIZE:%d,PKTS:%d" % (total_size, total_pkts)
                try:
                    i2c.send(response + "\n")
                except:
                    pass

                # Enviar paquete por paquete por SPI
                for i in range(total_pkts):
                    inicio = i * paquete_size
                    fin = inicio + paquete_size
                    fragmento = datos_jpeg[inicio:fin]

                    # Si es el último paquete es menor a 1024 rellenar con ceros
                    if len(fragmento) < paquete_size:
                        fragmento += bytearray(paquete_size - len(fragmento))

                    print("Esperando reloj del Maestro para paquete", i + 1)
                    try:
                        # Mandamos el fragmento por SPI
                        # si el maestro falla.
                        spi.send(fragmento, timeout=2000)# Deben coincidir los timeout para evitar errores de envio
                    except Exception as e:
                        print("Error enviando paquete SPI:", e)
                        break

                print("Transmisión de imagen finalizada.")

                # Se vacia la respuesta para evitar enviar datos por I2C luego del bucle.
                response = ""

            # ------ FUNCIONES EXTRAS -------
            elif cmd == 'B':
                if brightness_val < MAX_VAL:
                    brightness_val += 1
                    sensor.set_brightness(brightness_val)
                response = "BRIGHT {}".format(brightness_val)

            elif cmd == 'b':
                if brightness_val > MIN_VAL:
                    brightness_val -= 1
                    sensor.set_brightness(brightness_val)
                response = "BRIGHT {}".format(brightness_val)

            elif cmd == 'C':
                if contrast_val < MAX_VAL:
                    contrast_val += 1
                    sensor.set_contrast(contrast_val)
                response = "CONTRAST {}".format(contrast_val)

            elif cmd == 'c':
                if contrast_val > MIN_VAL:
                    contrast_val -= 1
                    sensor.set_contrast(contrast_val)
                response = "CONTRAST {}".format(contrast_val)

            elif cmd == 's':
                response = "C:{} B:{} M:{}".format(
                    contrast_val,
                    brightness_val,
                    mode
                )
            elif cmd == 'i':
                response = "{}{:.2f}{}{}".format(
                    minimize_label(best_label),
                    best_score,
                    detected_flag,
                    saved_flag
                )
            elif cmd == 'p':
                response = "PING"

            i2c.send("{}{}".format(response_status, response))

    except OSError:
        pass
