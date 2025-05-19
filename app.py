from flask import Flask, request, render_template, redirect
import json
from datetime import datetime
import os
import re

app = Flask(__name__)
LOG_FILE = 'data.json'

# Ensure log file exists
if not os.path.exists(LOG_FILE):
    with open(LOG_FILE, 'w') as f:
        json.dump([], f)

def get_dustbin_id_from_message(message_str):
    """Extracts Dustbin ID from a log message string."""
    match = re.search(r"Dustbin ID: (Dustbin\d+)", message_str)
    if match:
        return match.group(1)
    return None


@app.route('/')
def dashboard():
    with open(LOG_FILE, 'r') as f:
        try:
            all_logs = json.load(f)
        except json.JSONDecodeError:
            all_logs = [] # Handle empty or malformed JSON

    # Extract unique dustbin IDs for the filter dropdown
    all_dustbin_ids = set()
    for log in all_logs:
        dustbin_id = get_dustbin_id_from_message(log.get('message', ''))
        if dustbin_id:
            all_dustbin_ids.add(dustbin_id)
    unique_dustbin_ids = sorted(list(all_dustbin_ids))

    selected_dustbin_id_filter = request.args.get('dustbin_id')
    
    if selected_dustbin_id_filter:
        filtered_logs = [
            log for log in all_logs 
            if get_dustbin_id_from_message(log.get('message', '')) == selected_dustbin_id_filter
        ]
    else:
        filtered_logs = all_logs

    return render_template(
        'dashboard.html', 
        logs=filtered_logs[::-1],  # show latest first
        all_dustbin_ids=unique_dustbin_ids,
        selected_dustbin_id=selected_dustbin_id_filter
    )

# @app.route("/notify", methods=["POST"])
# def notify():
#     msg = request.form.get("msg")
#     print("Message received:", msg)
#     return "OK"

@app.route('/notify')
def notify():
    msg = request.args.get('msg')
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    log_entry = {
        "message": msg,
        "timestamp": timestamp
    }

    # Read current data, append, and write back
    # This is not perfectly safe for high concurrency but okay for this example
    try:
        with open(LOG_FILE, 'r') as f:
            data = json.load(f)
    except (FileNotFoundError, json.JSONDecodeError):
        data = []
        
    data.append(log_entry)
    
    with open(LOG_FILE, 'w') as f: # Changed to 'w' to overwrite with updated data
        json.dump(data, f, indent=2)

    return 'Notification received!'

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')