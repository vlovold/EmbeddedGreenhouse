from flask import Flask, render_template, request

from data import uart_read, get_data,uart_init

#ser = uart_init()
app = Flask(__name__)
#app.run(host="0.0.0.0", port=5000)
actuators = {"pump": 0, "fan": 0, "led": 0}
mode = "MANUAL"
@app.route("/")
def index():
    return render_template("index.html")

@app.route("/data")
def data():
#    msg = uart_read(ser)
#    parsed_data = get_data(msg)
    parsed_data = {
        "TEMP": 20,
        "HUM": 30,
        "BRIGHT": 600,
        "SOIL": 40,
        "FAN": 800
    }
    return parsed_data

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
    print("LED:", actuators["led"], "PUMP:", actuators["pump"], "FAN:", actuators["fan"])
    return {"status": "ok"}

@app.route("/mode", methods=["POST"])
def set_mode():
    global mode

    data = request.get_json()
    mode = data["mode"]

    print("Mode changed to:", mode)

    return {"status": "ok"}

if __name__ == "__main__":
    app.run(debug=True)