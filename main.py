from flask import Flask, render_template
from data import uart_read, get_data,uart_init

ser = uart_init()
app = Flask(__name__)
@app.route("/")
def index():
    return render_template("index.html")

@app.route("/data")
def data():
    msg = uart_read(ser)
    parsed_data = get_data(msg)
    return parsed_data


if __name__ == "__main__":
    app.run(debug=True)