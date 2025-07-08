#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

//All of the following macros are temporary for now 
// Motor A - BTS7960
#define RPWM_L 25
#define LPWM_L 26

// Motor B - BTS7960
#define RPWM_R 12
#define LPWM_R 13

// Stepper - TB6600
#define STEP_PIN 18
#define DIR_PIN  19
const int stepsPerMM = 1600;       // for 1/16 microstepping, 2mm pitch
const int rackHeightMM = 75;
volatile unsigned long lastInterruptTime = 0;

// IR sensors
#define leftIR  34
#define rightIR 35
#define IR_Side  14 //Connect only to that pin in ESP 32 which support interrupts 

volatile int colCounter = 0; //Counter to count the columns 
volatile int rowCounter = 0 ; //Counter to count the rows 
volatile bool colDetected = false;
volatile bool spotDetected = false;

volatile int prevRow = 0;
volatile int prevCol = 0;

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
  Serial.println("WiFi not connected. Attempting to connect...");
  connectToWiFi();
  
  if (wifi_connected) {
    connectToWebSocket();
  }

  //******* PIN CONFIGURATIONS *************
  // Setup BTS7960
  pinMode(RPWM_L, OUTPUT); pinMode(LPWM_L, OUTPUT);
  pinMode(RPWM_R, OUTPUT); pinMode(LPWM_R, OUTPUT);

  // Setup Stepper
  pinMode(STEP_PIN, OUTPUT); pinMode(DIR_PIN, OUTPUT);

    // IR sensors
  pinMode(leftIR, INPUT);
  pinMode(rightIR, INPUT);
  pinMode(IR_Side, INPUT);

  // Attach interrupt: FALLING = High to Low = detecting black
  attachInterrupt(digitalPinToInterrupt(IR_Side), sideIR_isr, FALLING);

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

bool reachRowCol(int row, int col) {
  Serial.printf("Processing coordinates: row=%d, col=%d\n", row, col);
  Serial.printf("Reaching column: %d" , col);

  bool columnReached = reachCol(col);
  bool rowReached = reachRow(row);

  if(columnReached && rowReached){

    //********************* CHOD BHANGRA ******************************//
    Serial.println("Reached desired rack position !!!");
    Serial.println("Enter next rack positoin from the dashboard");
    return true;

  }
  return false;
} 


/**************************************************************************** 
|                                                                           |
|************************* HELPER FUNCTIONS ********************************|
|                                                                           |
*****************************************************************************
*/

bool reachCol(int col){

  int colDiff = col - prevCol; 
  bool flag =false;

  if(colDiff > 0){
    while(colCounter != col){
      move_forward();
    }
    stop();
    colCounter =0;
    prevCol = col; 
    flag = true;
  }
  else{
    while(colCounter != col){
      move_backward();
    }
    stop();
    colCounter= 0;
    prevCol = col; 
    flag =true;
  }
  Serial.printf("Reached entered column value");
  return flag ;

}

bool reachRow(int row){
  int rowDiff = row - prevRow;
  bool flag = false; 
  int distanceMM = rowDiff * rackHeightMM;
  int totalSteps = abs(distanceMM) * stepsPerMM;

  bool direction = (rowDiff > 0); // true = up, false = down
  stepMotor(totalSteps, direction);
  flag = true;
  prevRow = row;
  return flag ;
  
}

void stepMotor(int steps, bool dir) {
  digitalWrite(DIR_PIN, dir);
  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(500); // Control speed
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(500);
  }
}

void IRAM_ATTR sideIR_isr() {
  unsigned long currentTime = millis();
  if ((currentTime - lastInterruptTime) > 200 && !spotDetected) {
    colCounter++;
    spotDetected = true;
    lastInterruptTime = currentTime;
  }
}

// Move forward with line following
void move_forward() {
  int leftValue = digitalRead(leftIR);
  int rightValue = digitalRead(rightIR);

  if (leftValue == LOW && rightValue == LOW) {
    // On track
    forward();
    spotDetected = false;
  }
  else if (leftValue == LOW && rightValue == HIGH) {
    // Drifted right, turn left
    turn_left();
  }
  else if (leftValue == HIGH && rightValue == LOW) {
    // Drifted left, turn right
    turn_right();
  }
  else {
    stop();
  }
}

// Move backward with line following
void move_backward() {
  int leftValue = digitalRead(leftIR);
  int rightValue = digitalRead(rightIR);

  if (leftValue == LOW && rightValue == LOW) {
    backward();
    spotDetected = false;
  }
  else if (leftValue == LOW && rightValue == HIGH) {
    turn_left_backward();
  }
  else if (leftValue == HIGH && rightValue == LOW) {
    turn_right_backward();
  }
  else {
    stop();
  }
}

// Motor control functions
void forward() {
  analogWrite(RPWM_L, 255);
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_R, 255);
  analogWrite(LPWM_R, 0);
}

void backward() {
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_L, 255);
  analogWrite(RPWM_R, 0);
  analogWrite(LPWM_R, 255);
}

void turn_left() {
  analogWrite(RPWM_L, 200);
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_R, 100);
  analogWrite(LPWM_R, 0);
}

void turn_right() {
  analogWrite(RPWM_L, 100);
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_R, 200);
  analogWrite(LPWM_R, 0);
}

void turn_left_backward() {
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_L, 200);
  analogWrite(RPWM_R, 0);
  analogWrite(LPWM_R, 100);
}

void turn_right_backward() {
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_L, 100);
  analogWrite(RPWM_R, 0);
  analogWrite(LPWM_R, 200);
}

void stop() {
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_R, 0);
  analogWrite(LPWM_R, 0);
}
