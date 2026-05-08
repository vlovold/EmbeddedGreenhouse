from flask import Flask, render_template, request, Response
from picamera2 import Picamera2
import cv2
from data import uart_read, get_data,uart_init, uart_USB_init

ser = uart_init()
ser_usb = uart_USB_init()
app = Flask(__name__)
actuators = {"pump": 0, "fan": 0, "led": 0}
sensors = {"TEMP": 0, "BRIGHT": 0, "HUM": 0, "FAN": 0, "SOIL": 0, "LED": 0}
mode = "MANUAL"
@app.route("/")
def index():
    return render_template("index.html")

@app.route("/data")
def data():
    global sensors

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

    return sensors

@app.route("/set", methods=["POST"])
def send_actuator():
    global actuators
    global mode
    if mode == "AUTO":
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

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True, use_reloader=False)