#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <esp32-hal-ledc.h>  // for the new LEDC API in Arduino-ESP32 v3.x
#include <Arduino.h>

#define RPWM_L 25
#define LPWM_L 26
#define RPWM_R 12
#define LPWM_R 13
#define STEP_PIN 18
#define DIR_PIN 19
#define leftIR 34
#define rightIR 35
#define IR_Side 14

const int stepsPerMM = 100;
const int rackHeightMM = 75;
volatile unsigned long lastInterruptTime = 0;
volatile int colCounter = 0;
volatile int rowCounter = 0;
volatile bool colDetected = false;
volatile bool spotDetected = false;
volatile int prevRow = 4;
volatile int prevCol = 0;
int leftValue = 0;
int rightValue = 0;
const int pwmFreq       = 1000;  // 1 kHz
const int pwmResolution = 8;     // 8-bit → 0–255

const char* ssid = "Local";
const char* password = "12233344440";
const char* relayServerIP = "192.168.137.215"; //your laptop ip
const int relayServerPort = 8080;

WebSocketsClient webSocket;

void IRAM_ATTR sideIR_isr();
bool reachRowCol(int row, int col);
bool reachCol(int col);
bool reachRow(int row);
void stepMotor(int steps, bool dir);
void move_forward();
void move_backward();
void forward();
void backward();
void turn_left();
void turn_right();
void turn_left_backward();
void turn_right_backward();
void stop();

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("Disconnected from relay server!");
      break;
      
    case WStype_CONNECTED:
      Serial.println("Connected to relay server!");
      webSocket.sendTXT("{\"type\":\"esp32_connected\"}");
      break;
      
    case WStype_TEXT: {
      Serial.printf("Received: %s\n", payload);
      
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        break;
      }
      
      if (doc.containsKey("row") && doc.containsKey("col")) {
        int row = doc["row"];
        int col = doc["col"];
        
        Serial.println("=== COORDINATES RECEIVED ===");
        Serial.printf("Row: %d\n", row);
        Serial.printf("Column: %d\n", col);
        Serial.printf("Position: [%d, %d]\n", row, col);
        Serial.println("===========================");
        
        bool result = reachRowCol(row, col);
        //bool result = (row + col) % 2 == 0;
        String response = "{\"result\":" + String(result ? "true" : "false") + "}";
        webSocket.sendTXT(response);
      }
      break;
    }
      
    case WStype_ERROR:
      Serial.println("WebSocket error!");
      break;
      
    case WStype_PING:
      Serial.println("Received ping");
      break;
      
    case WStype_PONG:
      Serial.println("Received pong");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  ledcAttach(RPWM_L, pwmFreq, pwmResolution);
  ledcAttach(LPWM_L, pwmFreq, pwmResolution);
  ledcAttach(RPWM_R, pwmFreq, pwmResolution);
  ledcAttach(LPWM_R, pwmFreq, pwmResolution);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(leftIR, INPUT);
  pinMode(rightIR, INPUT);
  pinMode(IR_Side, INPUT);
  attachInterrupt(digitalPinToInterrupt(IR_Side), sideIR_isr, RISING);
  
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  Serial.println("Connecting to relay server...");
  webSocket.begin(relayServerIP, relayServerPort, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  
  Serial.println("ESP32 ready to receive coordinates!");
}

void loop() {
  webSocket.loop();
}

bool reachRowCol(int row, int col) {
  Serial.printf("Processing coordinates: row=%d, col=%d\n", row, col);
  Serial.printf("Reaching column: %d", col);

  bool columnReached = reachCol(col);
  bool rowReached = reachRow(row);

  if (columnReached && rowReached) {
    Serial.println("Reached desired rack position !!!");
    Serial.println("Enter next rack position from the dashboard");
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


bool reachCol(int col) {
  int colDiff = col - prevCol;
  bool flag = false;

  while(colCounter != abs(colDiff)){
    if(colDiff>0) move_forward(); else move_backward();
    stop();
    colCounter = 0;
    prevCol = col;
    flag = true;
  }
  Serial.printf("Reached entered column value");
  return flag;
}

bool reachRow(int row) {
  int rowDiff = row - prevRow;
  bool flag = false;
  int distanceMM = rowDiff * rackHeightMM;
  int totalSteps = abs(distanceMM) * stepsPerMM;

  bool direction = (rowDiff < 0);
  stepMotor(totalSteps, direction);
  flag = true;
  prevRow = row;
  return flag;
}

void stepMotor(int steps, bool dir) {
  digitalWrite(DIR_PIN, dir);
  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(100);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(100);
  }
}

void IRAM_ATTR sideIR_isr() {
  unsigned long currentTime = millis();
  if ((currentTime - lastInterruptTime) > 200 && !spotDetected) {
    colCounter++;
    Serial.println("colCounter Value: ");
    Serial.println(colCounter);
    spotDetected = true;
    lastInterruptTime = currentTime;
  }
}
void move_forward() {
  leftValue  = digitalRead(leftIR);
  rightValue = digitalRead(rightIR);
  Serial.printf("Sensors: %d, %d → ", leftValue, rightValue);

  if (leftValue==LOW && rightValue==LOW) {
    Serial.println("stopped");
    stop();
  }
  else if (leftValue==HIGH && rightValue==LOW) {
    Serial.println("turn left");
    turn_left();
  }
  else if (leftValue==LOW && rightValue==HIGH) {
    Serial.println("turn right");
    turn_right();
  }
  else {
    Serial.println("forward");
    forward();
  }
}

void move_backward() {
  leftValue  = digitalRead(leftIR);
  rightValue = digitalRead(rightIR);

  if (leftValue==LOW && rightValue==LOW) {
    Serial.println("stopped");
    stop();
  }
  else if (leftValue==HIGH && rightValue==LOW) {
    Serial.println("turn right (back)");
    turn_right_backward();
  }
  else if (leftValue==LOW && rightValue==HIGH) {
    Serial.println("turn left (back)");
    turn_left_backward();
  }
  else {
    Serial.println("backward");
    backward();
  }
}

void forward() {
  ledcWrite(RPWM_L, 0);
  ledcWrite(LPWM_L, 200);
  ledcWrite(RPWM_R, 200);
  ledcWrite(LPWM_R, 0);
}

void backward() {
  ledcWrite(RPWM_L, 200);
  ledcWrite(LPWM_L, 0);
  ledcWrite(RPWM_R, 0);
  ledcWrite(LPWM_R, 200);
}

void turn_left() {
  ledcWrite(RPWM_L, 0);
  ledcWrite(LPWM_L, 0);
  ledcWrite(RPWM_R, 200);
  ledcWrite(LPWM_R, 0);
}

void turn_right() {
  ledcWrite(RPWM_L, 0);
  ledcWrite(LPWM_L, 200);
  ledcWrite(RPWM_R, 0);
  ledcWrite(LPWM_R, 0);
}

void turn_left_backward() {
  ledcWrite(RPWM_L, 0);
  ledcWrite(LPWM_L, 0);
  ledcWrite(RPWM_R, 0);
  ledcWrite(LPWM_R, 200);
}

void turn_right_backward() {
  ledcWrite(RPWM_L, 200);
  ledcWrite(LPWM_L, 0);
  ledcWrite(RPWM_R, 0);
  ledcWrite(LPWM_R, 0);
}

void stop() {
  ledcWrite(RPWM_L, 0);
  ledcWrite(LPWM_L, 0);
  ledcWrite(RPWM_R, 0);
  ledcWrite(LPWM_R, 0);
}