import websocket, time, rel, sys, json, requests

if len(sys.argv) <= 1:
    print("Usage: python3 control_services.py <WEBSOCKET_ENDPOINT> [LIST OF DEVICES TO CHECK]")
    sys.exit(1)

# Default websocket endpoint should be: "wss://valorantcrazyclips69.com/deviceV2"

WEBSOCKET_ENDPOINT = sys.argv[1]
DEVICE_NAMES = sys.argv[2:]
print("Websocket Endpoint: " + WEBSOCKET_ENDPOINT)
BASE_ENDPOINT = "http://localhost:8080"

def send_post_request(URL):
    print("Executing POST Request to URL: " + request_url)
    return requests.post(URL).text

def send_get_request(URL):
    return requests.get(URL).text

def build_request_url(parsed_message):
    device_id = parsed_message["deviceId"]
    device_secret = parsed_message["deviceSecret"]
    action = parsed_message["action"]
    return BASE_ENDPOINT + "/" + action + "/" + device_id + "/" + device_secret

def execute_action(parsed_action):
    print("Executing: ", parsed_action)
    request_url = build_request_url(parsed_action)
    requests.post(request_url)

def get_current_status(deviceId, deviceSecret):
    return send_get_request(build_request_url({"deviceId": deviceId, "deviceSecret": deviceSecret, "action": "status"}))

def send_current_status(ws, parsedAction):
    status = get_current_status(parsedAction["deviceId"], parsedAction["deviceSecret"])
    message = json.dumps({"type": "INFO", "deviceId": parsedAction["deviceId"], "deviceSecret": parsedAction["deviceSecret"], "status": status})
    print("Sending message to websocket: " + message)
    ws.send(message)

def on_message(ws, message):
    print("Received Message: " + message)
    try:
        parsed = json.loads(message)
        execute_action(parsed)
        send_current_status(ws, parsed)
    except Exception as e:
        print(f"Error when trying to process message: {e}")
        pass

def connected(ws):
    print("Websocket Connected")
    time.sleep(1)
    for name in DEVICE_NAMES:
        ws.send(json.dumps({
            "type": "INTEREST",
            "deviceId": name
        }))

if __name__ == "__main__":
    ws = websocket.WebSocketApp(WEBSOCKET_ENDPOINT, on_message=on_message, on_open=connected)

    ws.run_forever(dispatcher=rel, reconnect=5)  # Set dispatcher to automatic reconnection, 5 second reconnect delay if connection closed unexpectedly
    rel.signal(2, rel.abort)  # Keyboard Interrupt
    rel.dispatch()