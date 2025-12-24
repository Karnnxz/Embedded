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
  
  gate.setPeriodHertz(50);
  gate.attach(SERVO_PIN, 500, 2400);
  gate.write(CLOSE_ANGLE);
  
  Serial.println("System Ready");
}

void loop() {
  int v1 = analogRead(LDR1);
  int v2 = analogRead(LDR2);

  // Case: Car in
  if (v1 < DARK_THRESHOLD) {
    Serial.println("LDR1 BLOCKED -> OPENING GATE");
    gate.write(OPEN_ANGLE);
    
    // รอจนกว่ารถจะพ้น LDR1
    while(analogRead(LDR1) < DARK_THRESHOLD) { delay(50); } 
    Serial.println("Passed LDR1... Waiting for LDR2 to close");

    // รอจนกว่ารถจะไปบัง LDR2 เพื่อสั่งปิด ( 8 sec )
    unsigned long startTime = millis();
    while(analogRead(LDR2) > DARK_THRESHOLD) {
      if (millis() - startTime > 8000) break;
      delay(20);
    }

    Serial.println("LDR2 BLOCKED -> CLOSING GATE");
    delay(1000);
    gate.write(CLOSE_ANGLE);
    delay(2000);
  }

  // Case: Car out
  else if (v2 < DARK_THRESHOLD) {
    Serial.println("LDR2 BLOCKED -> OPENING GATE");
    gate.write(OPEN_ANGLE);

    // รอจนกว่ารถจะพ้น LDR2
    while(analogRead(LDR2) < DARK_THRESHOLD) { delay(50); }
    Serial.println("Passed LDR2... Waiting for LDR1 to close");

    // รอจนกว่ารถจะไปบัง LDR1 เพื่อสั่งปิด
    unsigned long startTime = millis();
    while(analogRead(LDR1) > DARK_THRESHOLD) {
      if (millis() - startTime > 8000) break;
      delay(20);
    }

    Serial.println("LDR1 BLOCKED -> CLOSING GATE");
    delay(1000);
    gate.write(CLOSE_ANGLE);
    delay(2000);
  }

  delay(100);
}
