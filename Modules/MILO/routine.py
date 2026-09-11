import sensor
import ml
import gc
import pyb
from pyb import I2C

# Init sensor
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.set_windowing((240, 240))
sensor.skip_frames(time=2000)


def minimize_label(label: str) -> str:
    if label == "Cloudy_Low":
        return 1
    elif label == "Cloudy_Medium":
        return 2
    elif label == "Cloudy_High":
        return 3
    else:
        return 4


i2c = I2C(2, I2C.SLAVE, addr=0x42)

req_buff = bytearray(6)
res_buff = bytearray(6)


def writePacket(
    subsystem,
    status,
    module,
    command,
    param0,
    param1,
):
    global res_buff

    res_buff[0] = subsystem  # SUBSYSTEMS
    res_buff[1] = status  # Status OK
    res_buff[2] = module  # Portenta
    res_buff[3] = command  # PING ACK
    res_buff[4] = param0  # Param 0
    res_buff[5] = param1  # Param1


PROTOCOL_SUBSYSTEMS = ord("S")
SUBSYSTEM_MILO = ord("M")
SUBSYSTEM_PORTENTA = ord("P")

# ================= COMMANDS
PING_ACK = 15
TAKE_PICTURE_ACK = 17
SEND_DATA_ACK = 19


def load_labels(path):
    return [line.rstrip("\n") for line in open(path)]


net = None
current_model = 0
mode = 0

best_label = "Error"
best_score = 0


def load_model(model_id):
    global net, current_model, labels
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


load_model(1)

while True:
    try:
        i2c.recv(req_buff, timeout=10)
        protocol = req_buff[0]
        status = req_buff[1]
        subsys = req_buff[2]
        cmd = req_buff[3]
        param0 = req_buff[4]
        param1 = req_buff[5]

        if subsys != SUBSYSTEM_MILO:  # ignore not milo messages
            continue

        if cmd == 14:  # MILO PING
            writePacket(PROTOCOL_SUBSYSTEMS, 0, SUBSYSTEM_PORTENTA, PING_ACK, 0, 0)
        elif cmd == 16:  # MILO TAKE PICTURE
            img = sensor.snapshot()
            rtc = pyb.RTC()
            date_time = rtc.datetime()
            name = "images/capture_%d%d%d_%d%d%d.jpg" % (
                date_time[0],
                date_time[1],
                date_time[2],
                date_time[4],
                date_time[5],
                date_time[6],
            )
            img.save(name, quality=100)
            gc.collect()
            print("Guardada:", name)

            try:
                predictions = net.predict([img])[0].flatten().tolist()
                best_label, best_score = max(
                    zip(labels, predictions), key=lambda x: x[1]
                )

            except Exception as e:
                print("Error inferencia:", e)
                best_label = "Error"
                best_score = 0

            writePacket(
                PROTOCOL_SUBSYSTEMS, 0, SUBSYSTEM_PORTENTA, TAKE_PICTURE_ACK, 0, 0
            )
        elif cmd == 18:  # MILO SEND DATA
            writePacket(
                PROTOCOL_SUBSYSTEMS,
                0,
                SUBSYSTEM_PORTENTA,
                SEND_DATA_ACK,
                minimize_label(best_label),
                0,
            )

        # Send the response buffer
        i2c.send(res_buff)

    except OSError:
        pass
