from flask import Flask, render_template, jsonify, request

app = Flask(__name__)

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/data")
def data():
    return {"sensor": 42}

if __name__ == "__main__":
    app.run(debug=True)