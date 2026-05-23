from flask import Flask, render_template, request, Response
from picamera2 import Picamera2
import cv2
from data import uart_read, get_data,uart_init, uart_USB_init
from controller import humidity_controller
import pandas as pd
from datetime import datetime
import signal
import sys
import subprocess
import os

ser = uart_init()
ser_usb = uart_USB_init()
app = Flask(__name__)
actuators = {"pump": 0, "fan": 0, "led": 0}
sensors = {"TEMP": 0, "BRIGHT": 0, "HUM": 0, "FAN": 0, "SOIL": 0, "LED": 0}
mode = "MANUAL"
df = pd.DataFrame(columns=[
    "TIME", "TEMP", "HUM", "SOIL", "PUMP", "FAN", "LED"])
@app.route("/")
def index():
    return render_template("index.html")

@app.route("/data")
def data():
    global sensors
    global df

    # Read from USB UART
    msg_usb = uart_read(ser_usb)
    print("USB:", msg_usb)

    # Read from GPIO UART
    msg_gpio = uart_read(ser)
    print("GPIO:", msg_gpio)

    # Parse both messages
    parsed_usb = get_data(msg_usb)
    parsed_gpio = get_data(msg_gpio)

    # Merge dictionaries
    parsed_data = {**parsed_usb, **parsed_gpio}

    # Soil conversion
    if "SOIL" in parsed_data:
        parsed_data["SOIL"] = int((parsed_data["SOIL"] / 4095) * 100)

    # Update sensors
    for key in parsed_data:
        sensors[key] = parsed_data[key]

    # Lagre ny måling i dataframe
    new_row = {
        "TIME": datetime.now(),

        "TEMP": sensors["TEMP"],
        "HUM": sensors["HUM"],
        "SOIL": sensors["SOIL"],

        "PUMP": actuators["pump"],
        "FAN": actuators["fan"],
        "LED": actuators["led"]
    }

    df.loc[len(df)] = new_row

    print(df.tail())

    return sensors

@app.route("/set", methods=["POST"])
def send_actuator():
    global actuators
    global mode

    if mode == "AUTO":
        fan_value = humidity_controller(sensors["HUM"], 40)
        actuators["fan"] = fan_value

        msg_usb = (
            f"FAN:{int((fan_value / 1400) * 255)}"
            f"-PUMP:{int((actuators['pump'] / 100) * 255)}\r\n"
        )

        ser_usb.write(msg_usb.encode("utf-8"))
        print(fan_value)
        return {"status": "ignored (AUTO mode)"}
    data = request.get_json()

    if "pump" in data:
        actuators["pump"] = data["pump"]
    if "fan" in data:
        actuators["fan"] = data["fan"]
    if "led" in data:
        actuators["led"] = data["led"]
    fan = int(actuators["fan"])
    pump = int(actuators["pump"])
#    print("LED:" + str(actuators["led"]))
    msg = ("LED:" + str(actuators["led"]) + "\r\n")
    msg_usb = (
        f"FAN:{int((fan / 1400) * 255)}"
        f"-PUMP:{int((pump / 100) * 255)}\r\n"
    )
    ser.write(msg.encode('utf-8'))
    ser_usb.write(msg_usb.encode('utf-8'))
    print(msg)
    print(msg_usb)
    return {"status": "ok"}
@app.route("/mode", methods=["POST"])
def set_mode():
    global mode

    data = request.get_json()
    mode = data["mode"]

    print("Mode changed to:", mode)

    return {"status": "ok"}

picam2 = Picamera2()
camera_config = picam2.create_preview_configuration(
    main={"format": "RGB888"}
)
picam2.configure(camera_config)
picam2.start()
picam2.set_controls({
    "AeEnable": True,
    "AwbEnable": True,
    "Brightness": 0.0,
    "Contrast": 1.1,
    "Saturation": 1.1
})
def generate_frames():
    while True:
        frame = picam2.capture_array()

        ret, buffer = cv2.imencode('.jpg', frame)
        frame = buffer.tobytes()

        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
@app.route("/cam")
def video():
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

def git_save(filename):
    subprocess.run(["git", "add", filename])
    subprocess.run(["git", "commit", "-m", "Update greenhouse log"])
    subprocess.run(["git", "push"])

def save_log():
    global df

    print("Saving log...")

    os.makedirs("data", exist_ok=True)

    filename = datetime.now().strftime(
        "data/log_%Y%m%d_%H%M%S.csv"
    )

    df.to_csv(filename, index=False)
    print(f"CSV saved as {filename}")

    git_save(filename)

def shutdown_handler(sig, frame):
    save_log()
    sys.exit(0)

signal.signal(signal.SIGINT, shutdown_handler)
signal.signal(signal.SIGTERM, shutdown_handler)

if __name__ == "__main__":
    try:
        app.run(
            host="0.0.0.0",
            port=5000,
            debug=True,
            use_reloader=False
        )
    finally:
        save_log()

