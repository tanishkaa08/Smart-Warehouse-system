
#define RPWM_L 25
#define LPWM_L 26
#define RPWM_R 12
#define LPWM_R 13
#define leftIR 34
#define rightIR 35
#define DELAY_TIME 10

int leftValue = 0;
int rightValue = 0;
const int pwmFreq = 1000;     // 1 kHz PWM frequency
const int pwmChannel = 0;     // Choose channel 0–15
const int pwmResolution = 8; 

void setup() {
  Serial.begin(115150);
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);  // Configure

  // Initialize motor pins
  pinMode(RPWM_L, OUTPUT);
  pinMode(LPWM_L, OUTPUT);
  pinMode(RPWM_R, OUTPUT);
  pinMode(LPWM_R, OUTPUT);
  
  // Initialize IR sensor pins
  pinMode(leftIR, INPUT);
  pinMode(rightIR, INPUT);
  
  Serial.println("Line Following Robot Initialized!");
  Serial.println("Place robot on the line and it will start following...");
  delay(1500); // Give time to position robot
}

void loop() {
  // Execute line following logic using the same implementation as esp32.ino
  move_backward();
  
  // Small delay for stability
  delay(DELAY_TIME);
}

void move_forward() {
  int leftValue = digitalRead(leftIR);
  int rightValue = digitalRead(rightIR);
  Serial.print("Value detected ");
  Serial.print(leftValue);
  Serial.print(",");
  Serial.println(rightValue);

  if (leftValue == LOW && rightValue == LOW) { //low = 0 = detecting white, high = 1 = detecting black
    Serial.println("stopped");
    stop();
  } else if (leftValue == HIGH && rightValue == LOW) {
    Serial.println("Turning left");
    turn_left();
  } else if (leftValue == LOW && rightValue == HIGH) {    
    Serial.println("Turning right");
    turn_right();
  } else {
    Serial.println("Moving forward");
    forward();
  }
}

void move_backward() {
  int leftValue = digitalRead(leftIR);
  int rightValue = digitalRead(rightIR);

  if (leftValue == LOW && rightValue == LOW) {
    Serial.println("stopped");
    stop();
  } else if (leftValue == HIGH && rightValue == LOW) {
    Serial.println("Turning right");
    turn_right_backward();
  } else if (leftValue == LOW && rightValue == HIGH) {
    Serial.println("Turning left");
    turn_left_backward();
  } else {
    Serial.println("Moving forward");
    backward();
  }
}

void forward() {
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_L, 150);
  analogWrite(RPWM_R, 150);
  analogWrite(LPWM_R, 0);
}

void backward() {
  analogWrite(RPWM_L, 150);
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_R, 0);
  analogWrite(LPWM_R, 150);
}

void turn_left() {
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_R, 150);
  analogWrite(LPWM_R, 0);
}

void turn_right() {
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_L, 150);
  analogWrite(RPWM_R, 0);
  analogWrite(LPWM_R, 0);
}

void turn_left_backward() {
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_R, 0);
  analogWrite(LPWM_R, 150);
}

void turn_right_backward() {
  analogWrite(RPWM_L, 150);
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_R, 0);
  analogWrite(LPWM_R, 0);
}

void stop() {
  analogWrite(RPWM_L, 0);
  analogWrite(LPWM_L, 0);
  analogWrite(RPWM_R, 0);
  analogWrite(LPWM_R, 0);
}

// Function to print sensor values for debugging
void printSensorValues() {
  Serial.print("Left IR: ");
  Serial.print(leftValue);
  Serial.print(" | Right IR: ");
  Serial.println(rightValue);
} 