from flask import Flask, render_template
from data import usart_read
#testing data
sensor_data = {
    "temperature": 24,
    "soil_humidity": 2,
    "air_humidity": 4,
    "brightness": 600,
    "fan_speed": 400
}

app = Flask(__name__)
@app.route("/")
def index():
    return render_template("index.html")

@app.route("/data")
def data():
    return {
        "temp": usart_read(sensor_data, "temperature"),
        "soil": usart_read(sensor_data, "soil_humidity"),
        "air": usart_read(sensor_data, "air_humidity"),
        "brightness": usart_read(sensor_data, "brightness"),
        "fanspeed": usart_read(sensor_data, "fan_speed")
    }

if __name__ == "__main__":
    app.run(debug=True)