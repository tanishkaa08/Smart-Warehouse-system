#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#define RPWM_L 25
#define LPWM_L 26
#define RPWM_R 12
#define LPWM_R 13
#define STEP_PIN 18
#define DIR_PIN 19
#define leftIR 34
#define rightIR 35
#define IR_Side 14

const int stepsPerMM = 1600;
const int rackHeightMM = 75;
volatile unsigned long lastInterruptTime = 0;
volatile int colCounter = 0;
volatile int rowCounter = 0;
volatile bool colDetected = false;
volatile bool spotDetected = false;
volatile int prevRow = 0;
volatile int prevCol = 0;

const char* ssid = "Local";
const char* password = "12233344440";
const char* relayServerIP = "192.168.137.163"; //your laptop ip
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
        
        //bool result = reachRowCol(row, col); UNCOMMENT THIS when you have the motor working
        bool result = (row + col) % 2 == 0;
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
  
  pinMode(RPWM_L, OUTPUT);
  pinMode(LPWM_L, OUTPUT);
  pinMode(RPWM_R, OUTPUT);
  pinMode(LPWM_R, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(leftIR, INPUT);
  pinMode(rightIR, INPUT);
  pinMode(IR_Side, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(IR_Side), sideIR_isr, FALLING);
  
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

  if (colDiff > 0) {
    while (colCounter != col) {
      move_forward();
    }
    stop();
    colCounter = 0;
    prevCol = col;
    flag = true;
  } else {
    while (colCounter != col) {
      move_backward();
    }
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

  bool direction = (rowDiff > 0);
  stepMotor(totalSteps, direction);
  flag = true;
  prevRow = row;
  return flag;
}

void stepMotor(int steps, bool dir) {
  digitalWrite(DIR_PIN, dir);
  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(500);
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

void move_forward() {
  int leftValue = digitalRead(leftIR);
  int rightValue = digitalRead(rightIR);

  if (leftValue == LOW && rightValue == LOW) {
    forward();
    spotDetected = false;
  } else if (leftValue == LOW && rightValue == HIGH) {
    turn_left();
  } else if (leftValue == HIGH && rightValue == LOW) {
    turn_right();
  } else {
    stop();
  }
}

void move_backward() {
  int leftValue = digitalRead(leftIR);
  int rightValue = digitalRead(rightIR);

  if (leftValue == LOW && rightValue == LOW) {
    backward();
    spotDetected = false;
  } else if (leftValue == LOW && rightValue == HIGH) {
    turn_left_backward();
  } else if (leftValue == HIGH && rightValue == LOW) {
    turn_right_backward();
  } else {
    stop();
  }
}

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
