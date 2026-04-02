from flask import Flask, render_template, jsonify, request
from data import usart_read
#testing data
sensor_data = {
    "temperature": 24,
    "soil_humidity": 0.7,
    "air_humidity": 1,
    "brightness": 600,
    "fan_speed": 400
}




app = Flask(__name__)
@app.route("/")
def index():
    data_temperature = 30
    return render_template("index.html",
                           data_temp=data_temperature)

@app.route("/data")
def data():
    return {
        "temp" : usart_read(sensor_data, "temperature"),
        "soil humidity" : usart_read(sensor_data, "soil_humidity")
    }


if __name__ == "__main__":
    app.run(debug=True)