// Arduino Nano: 3x HC-SR04 (F,L,R) + 2 motors via L298N (left motor, right motor)
// Adjust pins to match your wiring

// Motor pins (L298N #1 controls left motor, L298N #2 controls right motor)
const uint8_t IN1_L = 2;
const uint8_t IN2_L = 3;
const uint8_t ENA    = 9;   // PWM for left motor

const uint8_t IN1_R = 4;
const uint8_t IN2_R = 5;
const uint8_t ENB    = 10;  // PWM for right motor

// Ultrasonic pins
const uint8_t TRIG_F = 6;
const uint8_t ECHO_F = 7;
const uint8_t TRIG_L = 8;
const uint8_t ECHO_L = 11;
const uint8_t TRIG_R = 12;
const uint8_t ECHO_R = A0;

const unsigned long SERIAL_PERIOD_MS = 150; // send readings ~6-7 Hz
unsigned long lastSerial = 0;

void setup() {
  Serial.begin(115200);
  // motor pins
  pinMode(IN1_L, OUTPUT); pinMode(IN2_L, OUTPUT); pinMode(ENA, OUTPUT);
  pinMode(IN1_R, OUTPUT); pinMode(IN2_R, OUTPUT); pinMode(ENB, OUTPUT);
  stopMotors();

  // ultrasound pins
  pinMode(TRIG_F, OUTPUT); pinMode(ECHO_F, INPUT);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);

  digitalWrite(TRIG_F, LOW); digitalWrite(TRIG_L, LOW); digitalWrite(TRIG_R, LOW);
  delay(50);
}

// helper: read one HC-SR04 (returns distance cm, or 999 on timeout)
long readHCSR04(uint8_t trigPin, uint8_t echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout (~5m)
  if (duration == 0) return 999; // timeout
  long dist = duration * 0.0343 / 2.0;
  if (dist > 600) return 999;
  return dist;
}

void forwardMotor(int pwm=140) {
  // left forward
  digitalWrite(IN1_L, HIGH); digitalWrite(IN2_L, LOW);
  analogWrite(ENA, pwm);
  // right forward
  digitalWrite(IN1_R, HIGH); digitalWrite(IN2_R, LOW);
  analogWrite(ENB, pwm);
}

void backwardMotor(int pwm=140) {
  digitalWrite(IN1_L, LOW); digitalWrite(IN2_L, HIGH);
  analogWrite(ENA, pwm);
  digitalWrite(IN1_R, LOW); digitalWrite(IN2_R, HIGH);
  analogWrite(ENB, pwm);
}

void turnLeft(int pwm=140) {
  // spin left wheel backward, right forward (in-place turn)
  digitalWrite(IN1_L, LOW); digitalWrite(IN2_L, HIGH);
  analogWrite(ENA, pwm);
  digitalWrite(IN1_R, HIGH); digitalWrite(IN2_R, LOW);
  analogWrite(ENB, pwm);
}

void turnRight(int pwm=140) {
  digitalWrite(IN1_L, HIGH); digitalWrite(IN2_L, LOW);
  analogWrite(ENA, pwm);
  digitalWrite(IN1_R, LOW); digitalWrite(IN2_R, HIGH);
  analogWrite(ENB, pwm);
}

void stopMotors() {
  digitalWrite(IN1_L, LOW); digitalWrite(IN2_L, LOW);
  analogWrite(ENA, 0);
  digitalWrite(IN1_R, LOW); digitalWrite(IN2_R, LOW);
  analogWrite(ENB, 0);
}

void loop() {
  // stagger ultrasonic triggers a bit to avoid crosstalk
  long dF = readHCSR04(TRIG_F, ECHO_F);
  delay(30);
  long dL = readHCSR04(TRIG_L, ECHO_L);
  delay(30);
  long dR = readHCSR04(TRIG_R, ECHO_R);

  unsigned long now = millis();
  if (now - lastSerial >= SERIAL_PERIOD_MS) {
    // send JSON-like
    Serial.print("{\"F\":"); Serial.print(dF);
    Serial.print(",\"L\":"); Serial.print(dL);
    Serial.print(",\"R\":"); Serial.print(dR);
    Serial.println("}");
    lastSerial = now;
  }

  // Check for Pi commands (non-blocking)
  while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 'F': forwardMotor(120); break;
      case 'B': backwardMotor(120); break;
      case 'L': turnLeft(110); break;
      case 'R': turnRight(110); break;
      case 'S': stopMotors(); break;
      default: /* ignore */ break;
    }
  }

  // small loop delay for stability
  delay(20);
}
