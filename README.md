# Smart Warehouse System

A smart warehouse automation system that enables real-time control of ESP32-powered robotic hardware through an intuitive HTML web interface. The system features a relay server architecture for seamless communication between web clients and embedded devices.

## System Architecture

```
┌─────────────────┐    WebSocket    ┌──────────────────┐    WebSocket    ┌─────────────────┐
│   HTML Client   │ ◄─────────────► │  Relay Server    │ ◄─────────────► │   ESP32 Client  │
│  (index.html)   │                 │ (relay_server.py)│                 │   (esp32.ino)   │
└─────────────────┘                 └──────────────────┘                 └─────────────────┘
```

## Features

- **Real-time Control**: Instant communication between web interface and ESP32 hardware
- **Grid-based Navigation**: 4x4 warehouse rack system with precise positioning
- **Motor Control**: Dual motor system with stepper motor for vertical movement
- **Sensor Integration**: IR sensors for position detection and obstacle avoidance
- **Web Interface**: Modern, responsive HTML5 interface with real-time status updates
- **Relay Architecture**: Scalable WebSocket relay server supporting multiple clients

## System Requirements

### Hardware
- ESP32 development board
- Dual DC motors with motor drivers
- Stepper motor for vertical movement
- IR sensors (3x) for position detection
- Power supply and wiring components

### Software
- Python 3.7+ with `websockets` library
- Arduino IDE with ESP32 board support
- Required Arduino libraries:
  - `ArduinoJson` (v6.x)
  - `WebSocketsClient`
  - `WiFi` (built-in)

## Setup Instructions

### 1. Network Configuration

**Configure your laptop's IP address in both files:**

**ESP32 Configuration (`esp32.ino` line 7):**
```cpp
const char* relayServerIP = "192.168.137.163"; // Replace with your laptop's IP
```

**HTML Client Configuration (`index.html` line 464):**
```javascript
const RELAY_SERVER_IP = '192.168.137.163'; // Replace with your laptop's IP
```

> **Note**: Ensure all devices are connected to the same WiFi network.

### 2. Relay Server Setup

```bash
# Install Python dependencies
pip install -r requirements.txt

# Start the relay server
python relay_server.py
```

The server will start on port 8080 and display connection logs for both HTML and ESP32 clients.

### 3. ESP32 Configuration & Flashing

**Prerequisites:**
- Install Arduino IDE with ESP32 board support
- Install required libraries via Library Manager:
  - `ArduinoJson` by Benoit Blanchon
  - `WebSocketsClient` by Markus Sattler

**Configuration Steps:**

1. **Update WiFi credentials** in `esp32.ino`:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

2. **Verify server IP** in `esp32.ino`:
   ```cpp
   const char* relayServerIP = "your_laptop_ip";
   ```

3. **Select correct board** in Arduino IDE:
   - Board: "ESP32 Dev Module"
   - Upload Speed: 115200

4. **Flash the ESP32** with the Arduino code

### 4. Launch Web Interface

Open `index.html` in your web browser and click **"Connect to Server"** to establish communication.

## Communication Protocol

### Connection Messages

**ESP32 Connection:**
```json
{
  "type": "esp32_connected",
  "message": "ESP32 WebSocket client connected"
}
```

**Status Updates:**
```json
{
  "type": "esp32_status",
  "connected": true,
  "message": "ESP32 connected"
}
```

### Control Commands

**HTML → ESP32 (via relay server):**
```json
{
  "row": 1,
  "col": 2
}
```

**ESP32 → HTML (via relay server):**
```json
{
  "result": true
}
```

**Movement Results:**
```json
{
  "type": "movement_result",
  "result": true,
  "message": "Movement completed successfully"
}
```

## Project Structure

```
Smart-Warehouse-system/
├── esp32.ino              # ESP32 Arduino code with motor control
├── index.html             # Web interface for warehouse control
├── relay_server.py        # WebSocket relay server
├── requirements.txt       # Python dependencies
├── README.md             # This documentation
├── backup/               # Backup files
└── working folder/       # Development and testing files
```

## Hardware Pin Configuration

| Component | ESP32 Pin | Function |
|-----------|-----------|----------|
| Left Motor PWM+ | GPIO 25 | Right PWM for left motor |
| Left Motor PWM- | GPIO 26 | Left PWM for left motor |
| Right Motor PWM+ | GPIO 12 | Right PWM for right motor |
| Right Motor PWM- | GPIO 13 | Left PWM for right motor |
| Stepper Step | GPIO 18 | Stepper motor step signal |
| Stepper Direction | GPIO 19 | Stepper motor direction |
| Left IR Sensor | GPIO 34 | Left position detection |
| Right IR Sensor | GPIO 35 | Right position detection |
| Side IR Sensor | GPIO 14 | Side obstacle detection |

## Troubleshooting

### Connection Issues

**ESP32 won't connect:**
- Verify WiFi credentials are correct
- Check that relay server IP matches your laptop's IP
- Ensure ESP32 and laptop are on the same network
- Check Serial Monitor for connection status

**HTML client can't connect:**
- Verify relay server is running (`python relay_server.py`)
- Check browser console for WebSocket errors
- Ensure RELAY_SERVER_IP is correctly set in `index.html`

**Relay server errors:**
- Check if port 8080 is available
- Verify Python websockets library is installed
- Check firewall settings

### Hardware Issues

**Motors not responding:**
- Verify motor driver connections
- Check power supply voltage
- Test individual motor functions
- Review pin assignments

**Sensors not working:**
- Check IR sensor wiring
- Verify sensor power supply
- Test sensor readings in Serial Monitor
- Clean sensor lenses if needed

### Code Issues

**Arduino compilation errors:**
- Ensure all required libraries are installed
- Check ESP32 board selection in Arduino IDE
- Verify ArduinoJson library version compatibility

**Python server errors:**
- Check Python version (3.7+ required)
- Verify websockets library installation
- Review server logs for specific error messages

## Development Notes

- The system uses a 4x4 grid coordinate system (rows 1-4, columns 1-4)
- Motor movement is currently simulated (`(row + col) % 2 == 0`)
- IR sensors provide position feedback for precise navigation
- The relay server supports multiple HTML clients simultaneously
- Real-time status updates are provided to all connected clients

## License

This project is open source and available under the MIT License.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.