#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const char* websocket_server = "your_laptop_ip";  // Change to your laptop's IP
const int websocket_port = 8080;                 // Change to your server port
const char* websocket_path = "/";                // WebSocket path

WebSocketsClient webSocket;

bool wifi_connected = false;
bool websocket_connected = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 WebSocket Client ===");
  Serial.println("Initializing...");
  
  if (wifi_connected) {
    connectToWebSocket();
  }else{
    Serial.println("WiFi not connected. Attempting to connect...");
    connectToWiFi();
  }
}

void loop() {
  if (websocket_connected) {
    webSocket.loop();
  }
  
  if (wifi_connected && !websocket_connected) {
    Serial.println("Attempting to reconnect to WebSocket...");
    connectToWebSocket();
    delay(5000);
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    wifi_connected = false;
    websocket_connected = false;
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    connectToWiFi();
  }
  
  delay(100);
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifi_connected = true;
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    wifi_connected = false;
    Serial.println("Failed to connect to WiFi");
  }
}

void connectToWebSocket() {
    Serial.printf("Connecting to WebSocket server: %s:%d%s\n", 
                  websocket_server, websocket_port, websocket_path);
    
    webSocket.begin(websocket_server, websocket_port, websocket_path);
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    
    webSocket.enableHeartbeat(15000, 3000, 2);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WebSocket] Disconnected");
      websocket_connected = false;
      break;
        
    case WStype_CONNECTED:
      Serial.printf("[WebSocket] Connected to: %s\n", payload);
      websocket_connected = true;
      
      webSocket.sendTXT("{\"type\":\"esp32_connected\",\"message\":\"ESP32 WebSocket client connected\"}");
      break;
        
    case WStype_TEXT:
      Serial.printf("[WebSocket] Received: %s\n", payload);
      handleIncomingMessage((char*)payload);
      break;
        
    case WStype_BIN:
      Serial.printf("[WebSocket] Received binary data, length: %u\n", length);
      break;
        
    case WStype_ERROR:
      Serial.printf("[WebSocket] Error: %s\n", payload);
      break;
        
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      Serial.println("[WebSocket] Fragment received");
      break;
        
    case WStype_PING:
      Serial.println("[WebSocket] Ping received");
      break;
        
    case WStype_PONG:
      Serial.println("[WebSocket] Pong received");
      break;
  }
}

void handleIncomingMessage(const char* message) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  if (doc.containsKey("row") && doc.containsKey("col")) {
    int row = doc["row"];
    int col = doc["col"];
    
    Serial.printf("Received coordinates: row=%d, col=%d\n", row, col);
    
    bool result = reach_row_col(row, col);
    
    Serial.printf("Placeholder function returned: %s\n", result ? "true" : "false");
    
    sendBooleanResponse(result);
  } else {
    Serial.println("Message doesn't contain valid coordinate data");
  }
}

void sendBooleanResponse(bool result) {
  StaticJsonDocument<100> response;
  response["result"] = result;
  
  String jsonString;
  serializeJson(response, jsonString);
  
  webSocket.sendTXT(jsonString);
  
  Serial.printf("Sent response: %s\n", jsonString.c_str());
}

bool reach_row_col(int row, int col) {
  Serial.printf("Processing coordinates: row=%d, col=%d\n", row, col);
  
  bool result = (row + col) % 2 == 0; //TODO: @Madhav add logic here
  
  delay(100);
  
  return result;
} 