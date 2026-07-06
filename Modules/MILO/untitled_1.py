# Untitled - By: Ervin Gomez - Fri May 8 2026
from pyb import SPI, Pin
import time

# Configuración del esclavo SPI2
spi = SPI(2, SPI.SLAVE, baudrate=1000000, polarity=0, phase=0)

# El buffer DEBE coincidir con la cantidad de bytes que envía el Arduino (16 bytes)
receive_buffer = bytearray(16)
send_buffer = bytearray(b'OpenMV_OK_123456') #Ajuste para coincidir los 16 bytes
print("Esperando datos SPI...")

while(True):
    try:
        # spi.send_recv espera a que el maestro envíe el reloj
        spi.send_recv(send_buffer, receive_buffer, timeout=10000)

        # Comprobar si recibimos algo diferente de 0 o vacío
        if receive_buffer[0] != 0:
            print("Recibido:", receive_buffer.decode('utf-8', 'ignore'))

            # Limpiar el buffer para la siguiente lectura
            for i in range(16):
                receive_buffer[i] = 0

    except OSError as e:
        # Manejar el timeout si el Arduino no envía nada en 10 segundos
        print("Esperando al Maestro...")
