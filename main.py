from flask import Flask, render_template
from data import uart_read, get_data,uart_init

#ser = uart_init()
app = Flask(__name__)
#app.run(host="0.0.0.0", port=5000)
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


if __name__ == "__main__":
    app.run(debug=True)