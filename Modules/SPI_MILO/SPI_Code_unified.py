import sensor
import time
import ml
import uos
import gc
import os
from pyb import SPI

# =========================================
# SENSOR
# =========================================

def init_sensor():

    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.set_windowing((240, 240))
    sensor.skip_frames(time=2000)

init_sensor()

# =========================================
# STORAGE
# =========================================

try:
    uos.mkdir("images")
except:
    pass

# =========================================
# SPI SLAVE
# MODE2 = polarity=1 phase=0
# =========================================

spi = SPI(2, SPI.SLAVE, polarity=1, phase=0)

# =========================================
# VARIABLES
# =========================================

net = None
labels = []

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

clock = time.clock()

# respuesta pendiente SPI
pending_response = b'READY\n'

# =========================================
# LOAD LABELS
# =========================================

def load_labels(path):

    return [
        line.rstrip('\n')
        for line in open(path)
    ]

# =========================================
# LOAD MODEL
# =========================================

def load_model(model_id):

    global net
    global current_model
    global labels

    print("Loading model...")

    if net is not None:

        net = None
        gc.collect()

    if model_id == 1:

        net = ml.Model(
            "Milo/EarthLimb/trained.tflite"
        )

        labels = load_labels(
            "Milo/EarthLimb/labels.txt"
        )

        print("EarthLimb loaded")

    elif model_id == 2:

        net = ml.Model(
            "Milo/Hypso/trained.tflite"
        )

        labels = load_labels(
            "Milo/Hypso/labels.txt"
        )

        print("Hypso loaded")

    elif model_id == 3:

        net = ml.Model(
            "Milo/Sentinel/trained.tflite"
        )

        labels = load_labels(
            "Milo/Sentinel/labels.txt"
        )

        print("Sentinel loaded")

    current_model = model_id

    gc.collect()

# =========================================
# MAIN LOOP
# =========================================

while True:

    saved_flag = 0

    # =====================================
    # INFERENCE
    # =====================================

    if mode in [1,2,3] and net is not None:

        clock.tick()

        img = sensor.snapshot()

        try:

            predictions = net.predict([img])[0] \
                .flatten().tolist()

            best_label, best_score = max(
                zip(labels, predictions),
                key=lambda x: x[1]
            )

        except Exception as e:

            print("Inference error:", e)

            best_label = "Error"
            best_score = 0

        if best_label == "Cloudy_Medium" \
            and best_score > 0.6:

            detected_flag = 1

            current_time = time.ticks_ms()

            if time.ticks_diff(
                current_time,
                last_detection_time
            ) > cooldown_ms:

                filename = "images/cloudy_%d.jpg" \
                    % current_time

                img.save(filename, quality=85)

                gc.collect()

                last_detection_time = current_time

                saved_flag = 1

        else:

            detected_flag = 0

    # =====================================
    # SPI RECEIVE
    # =====================================

    try:

        data = spi.recv(1, timeout=1000)

        if data:

            cmd = chr(data[0])
            print("CMD:", cmd)

            response = ""
            send_response = True    # Para respuesta de snapshot

            # =============================
            # MODELS
            # =============================

            if cmd == 'E' and current_model != 1:

                load_model(1)

                mode = 1

                response = "EARTHLIMB ON"

            elif cmd == 'H' and current_model != 2:

                load_model(2)

                mode = 2

                response = "HYPSO ON"

            elif cmd == 'T' and current_model != 3:

                load_model(3)

                mode = 3

                response = "SENTINEL ON"

            elif cmd == 'O':

                mode = 0

                net = None

                gc.collect()

                response = "MODELS OFF"

            # =============================
            # SNAPSHOT
            # =============================

            elif cmd == 'S':

                pending_response = b"SNAPSHOT SAVED\n"
                spi.send(pending_response)       # enviar respuesta inmediatamente
                send_response = False            # evitar que se envien dos respuestas en
                                                     # en un ciclo de snapshoot

                img = sensor.snapshot()

                filename = "images/manual_%d.jpg" \
                    % time.ticks_ms()

                img.save(filename, quality=85)

                gc.collect()

            # =============================
            # BRIGHTNESS
            # =============================

            elif cmd == 'B':

                if brightness_val < MAX_VAL:

                    brightness_val += 1

                    sensor.set_brightness(
                        brightness_val
                    )

                response = "B:%d" % brightness_val

            elif cmd == 'b':

                if brightness_val > MIN_VAL:

                    brightness_val -= 1

                    sensor.set_brightness(
                        brightness_val
                    )

                response = "B:%d" % brightness_val

            # =============================
            # CONTRAST
            # =============================

            elif cmd == 'C':

                if contrast_val < MAX_VAL:

                    contrast_val += 1

                    sensor.set_contrast(
                        contrast_val
                    )

                response = "C:%d" % contrast_val

            elif cmd == 'c':

                if contrast_val > MIN_VAL:

                    contrast_val -= 1

                    sensor.set_contrast(
                        contrast_val
                    )

                response = "C:%d" % contrast_val

            # =============================
            # STATUS
            # =============================

            elif cmd == 's':

                response = "C:{} B:{} M:{}" \
                    .format(
                        contrast_val,
                        brightness_val,
                        mode
                    )

            # =============================
            # INFERENCE RESULT
            # =============================

            elif cmd == 'i':

                response = "{},{:.2f},{},{}" \
                    .format(
                        best_label,
                        best_score,
                        detected_flag,
                        saved_flag
                    )

            # =============================
            # SEND IMAGE TO MCU
            # =============================

            elif cmd == 'R':

                img = img.scale(x_scale=0.5)
                datos_imagen = img.bytearray()
                weight_bytes = len(datos_imagen)

                print("Tamaño de imagen:", weight_bytes, "bytes")

                # Enviar tamaño primero
                pending_response = "%d\n" % weight_bytes
                spi.send(pending_response)
                send_response = False

                # Esperar que Arduino esté listo para recibir
                time.sleep_ms(300)

                # Enviar imagen en chunks
                CHUNK = 64
                offset = 0
                while offset < weight_bytes:
                    end = min(offset + CHUNK, weight_bytes)
                    spi.send(datos_imagen[offset:end])
                    offset = end                # ← fix aquí
                    time.sleep_ms(15)           # pausa entre chunks

                print("Imagen enviada.")

            # =============================
            # PING
            # =============================

            elif cmd == 'P':

                response = "ALIVE"

            else:

                response = "UNKNOWN"

            # =================================
            # PREPARE RESPONSE
            # =================================

            pending_response = bytes(
                response + "\n",
                'utf-8'
            )

            # =================================
            # SEND RESPONSE
            # =================================

            if send_response and response:
                pending_response = bytes(response + "\n", 'utf-8')
                spi.send(pending_response)

    except OSError:

        pass
