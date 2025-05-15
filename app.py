from flask import Flask, request, render_template, redirect
import json
from datetime import datetime
import os

app = Flask(__name__)
LOG_FILE = 'data.json'

# Ensure log file exists
if not os.path.exists(LOG_FILE):
    with open(LOG_FILE, 'w') as f:
        json.dump([], f)

@app.route('/')
def dashboard():
    with open(LOG_FILE, 'r') as f:
        logs = json.load(f)
    return render_template('dashboard.html', logs=logs[::-1])  # show latest first

@app.route('/notify')
def notify():
    msg = request.args.get('msg')
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    log_entry = {
        "message": msg,
        "timestamp": timestamp
    }

    with open(LOG_FILE, 'r+') as f:
        data = json.load(f)
        data.append(log_entry)
        f.seek(0)
        json.dump(data, f, indent=2)

    return 'Notification received!'

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')
