import serial
import time
def uart_init():
    ser = serial.Serial(port='/dev/serial0',
                        baudrate=9600,
                        timeout=2,
                        parity=serial.PARITY_NONE,
                        stopbits=serial.STOPBITS_ONE,
                        bytesize=serial.EIGHTBITS)
    time.sleep(0.1)
    return ser

def uart_read(ser):
    decoded_data = None
    if ser.inWaiting() > 0:
        data = ser.readline()
        decoded_data = data.decode('utf-8')
    return decoded_data

def get_data(msg):
    result = {}
    parts = msg.strip().split('-')

    for part in parts:
        key, value = part.split(':')
        result[key] = float(value)
    return result


