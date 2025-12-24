#include <ESP32Servo.h>

#define LDR1 34
#define LDR2 35
#define SERVO_PIN 32

#define DARK_THRESHOLD 1500
#define OPEN_ANGLE 90
#define CLOSE_ANGLE 0

Servo gate;

void setup() {
  Serial.begin(9600);

  pinMode(LDR1, INPUT);
  pinMode(LDR2, INPUT);

  gate.setPeriodHertz(50); // Servo PWM
  gate.attach(SERVO_PIN, 500, 2400);
  gate.write(CLOSE_ANGLE);

  Serial.println("System Ready");
}

void loop() {
  int ldr1 = analogRead(LDR1);
  int ldr2 = analogRead(LDR2);

  Serial.print("LDR 1: ");
  Serial.print(ldr1);
  Serial.print(" | LDR 2: ");
  Serial.println(ldr2);

  // LDR1 --> Servo Open
  if (ldr1 < DARK_THRESHOLD) {
    Serial.println(">> LDR A BLOCKED → GATE OPEN");
    gate.write(OPEN_ANGLE);
    delay(1500);     // กันสั่งซ้ำ
  }

  // LDR1 --> Servo Close
  if (ldr2 < DARK_THRESHOLD) {
    Serial.println(">> LDR B BLOCKED → GATE CLOSE");
    gate.write(CLOSE_ANGLE);
    delay(1500);
  }

  delay(200);
}
