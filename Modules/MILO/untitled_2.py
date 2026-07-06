import sensor
import time
import ml
import uos
import gc
import image
from pyb import I2C
from pyb import SPI, Pin

# Sensor
def init_sensor():
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA) #La imgaen es de 240 X 240
    sensor.set_windowing((240, 240))
    sensor.skip_frames(time=2000)


init_sensor()

#configuración SPI
spi = SPI(2, SPI.SLAVE, baudrate=1000000, polarity=0, phase=0)
print("Esperando datos SPI...")

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
receive_buffer = bytearray(16)
send_buffer = bytearray(16)

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
    spi_pending = False #Bandera para inicialización de SPI

    try:
        data = i2c.recv(1, timeout=10)

        if data:

            cmd = data.decode()
            response = ""

            # -------- MODELOS --------
            if cmd == 'E' and current_model != 1:
                load_model(1)
                mode = 1
                response = "MODEL EARTHLIMB ON"

            elif cmd == 'H' and current_model != 2:
                load_model(2)
                mode = 2
                response = "MODEL HYPSO ON"

            elif cmd == 'T' and current_model != 3:
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
                img = sensor.snapshot()
                filename = "images/manual_%d.jpg" % time.ticks_ms()
                img.save(filename, quality=85)
                gc.collect()

                tamaño = img.size()
                print("Tamaño de imagen (JPEG):", tamaño, "bytes")

                #----------------- ----------- PRUEBA 1 ----------------------------
                #----------------- Lectura de los 5 primeros pixeles ---------------
                for y in range(2):
                    for x in range(2):

                        #---------------- Valores en RGB565
                        #pixel = img.get_pixel(x,y)
                        #print("Píxel [X:", x, "Y:", y, "] -> Tipo:", type(pixel), "Valor:", pixel)

                        #------------ Valores en Hexadecimal ------------------------
                        r, g, b = img.get_pixel(x,y)
                        hex_val = "0x{:02x}{:02x}{:02x}".format(r, g, b)
                        print(hex_val)
                #-------------------------------------------------------------------

                response = "CAPTURE SAVED"

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
                response = "{},{:.2f},{},{}".format(
                    best_label,
                    best_score,
                    detected_flag,
                    saved_flag
                )
            elif cmd == 'p':
                response = "I'm alive!"

        #-------Envío de imagen por SPI---------------
            elif cmd == 'R':
                try:
                    # Nos aseguramos de tener una imagen, si no, tomamos una
                    if 'img' not in globals() and 'img' not in locals():
                        img = sensor.snapshot()

                    img_compressed = img.compress(quality=50)
                    weight_bytes = img_compressed.size()

                    print("Tamaño de imagen (JPEG):", weight_bytes, "bytes")

                    # Mandaremos el tamaño de la imagen convertido a string
                    msg_size = str(weight_bytes).encode('utf-8')

                    for i in range(16):
                        send_buffer[i] = 32 # Rellenamos con espacios

                    send_buffer[0:len(msg_size)] = msg_size # Insertamos el tamaño

                    response = "SPI_READY" # Le decimos al maestro que esta listo
                    spi_pending = True     # Activamos bandera

                except Exception as e:
                    response = "ERROR_SPI"
                    print("Error preparando SPI:", e)

            # Enviamos la respuesta por I2C PRIMERO
            if response != "":
                i2c.send(response + "\n")

            # Esperar el reloj del SPI
            if spi_pending:
                print("Esperando reloj SPI del Master...")
                # Deben de conincidir los Buffers
                spi.send_recv(send_buffer, receive_buffer, timeout=5000)

                print("Recibido por SPI:", receive_buffer.decode('utf-8', 'ignore'))
                # Limpiamos buffer
                for i in range(16):
                    receive_buffer[i] = 0
                spi_pending = False

    except OSError:
        pass
