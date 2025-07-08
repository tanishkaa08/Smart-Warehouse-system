# Smart Warehouse System

A WebSocket-based smart warehouse system that allows HTML client control of ESP32 hardware through a relay server.

## Architecture

```
HTML Client (index.html) <--> Relay Server (relay_server.py) <--> ESP32 Client (esp32.ino)
```

## Setup Instructions

### 1. Configure IP Addresses

**Set your laptop's IP address in:**
- `esp32.ino` line 7: `const char* websocket_server = "your_laptop_ip";`
- `index.html` line 464: `const RELAY_SERVER_IP = 'your_laptop_ip';`

Replace `"your_laptop_ip"` with your actual laptop's IP address.

### 2. Run the Relay Server

```bash
# Install dependencies
pip install -r requirements.txt

# Start the relay server
python relay_server.py
```

The server will start on port 8080 and listen for both HTML and ESP32 connections.

### 3. Configure and Flash ESP32

**Prerequisites:**
- Install ArduinoJson library in Arduino IDE
- Install WebSocketsClient library in Arduino IDE

**Configuration:**
1. Update WiFi credentials in `esp32.ino`:
   ```c
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

2. Update server IP in `esp32.ino` line 7:
   ```c
   const char* websocket_server = "your_laptop_ip";
   ```

3. Flash the ESP32 with the Arduino code

### 4. Open HTML Client

Open `index.html` in your web browser and click "Connect to Server".

## Communication Protocol

### ESP32 Connection Message:
```json
{
  "type": "esp32_connected",
  "message": "ESP32 WebSocket client connected"
}
```

### HTML to ESP32 (via relay server):
```json
{
  "row": 1,
  "col": 2
}
```

### ESP32 to HTML (via relay server):
```json
{
  "result": true
}
```

## File Structure

- `esp32.ino` - Arduino code for ESP32 WebSocket client
- `index.html` - HTML client interface
- `relay_server.py` - Python WebSocket relay server
- `requirements.txt` - Python dependencies
- `README.md` - This documentation

## Troubleshooting

- Ensure all devices are on the same WiFi network
- Check that the relay server is running before connecting clients
- Verify IP addresses are correctly configured in both `esp32.ino` and `index.html`
- Make sure Arduino libraries (ArduinoJson, WebSocketsClient) are installed
- Check Serial Monitor for ESP32 connection status and debug messages