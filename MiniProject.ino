#include <ESP32Servo.h>

#define LDR1 34
#define LDR2 35
#define SERVO_PIN 32

#define DARK_DETECT 1100
#define LIGHT_RELEASE 2000 

#define OPEN_ANGLE 90
#define CLOSE_ANGLE 0

Servo gate;

int carCount = 0; 

enum SystemState { IDLE, CAR_ENTERING, CAR_EXITING };
SystemState currentState = IDLE;

void setup() {
  Serial.begin(9600);
  pinMode(LDR1, INPUT);
  pinMode(LDR2, INPUT);
  gate.setPeriodHertz(50);
  gate.attach(SERVO_PIN, 500, 2400);
  gate.write(CLOSE_ANGLE);
  
  Serial.println("System Ready");
  Serial.print("Current Car in Parking: ");
  Serial.println(carCount);
}

void loop() {
  int v1 = analogRead(LDR1);
  int v2 = analogRead(LDR2);

  switch (currentState) {
    
    case IDLE:
      if (v1 < DARK_DETECT) {
        Serial.println("ENTRY DETECTED -> OPEN");
        gate.write(OPEN_ANGLE);
        currentState = CAR_ENTERING;
        delay(500); 
      }
      else if (v2 < DARK_DETECT) {
        Serial.println("EXIT DETECTED -> OPEN");
        gate.write(OPEN_ANGLE);
        currentState = CAR_EXITING;
        delay(500);
      }
      break;

    case CAR_ENTERING:
      // เมื่อผ่าน LDR2 (รถเข้าสำเร็จ)
      if (v2 < DARK_DETECT && v1 > LIGHT_RELEASE) {
        carCount++; // เพิ่มจำนวนรถ
        Serial.println("CAR ENTERED SUCCESS!");
        Serial.print("Total Cars: ");
        Serial.println(carCount);
        
        delay(1000); 
        gate.write(CLOSE_ANGLE);
        currentState = IDLE;
        delay(1500);
      }
      break;

    case CAR_EXITING:
      // เมื่อผ่าน LDR1 (รถออกสำเร็จ)
      if (v1 < DARK_DETECT && v2 > LIGHT_RELEASE) {
        if(carCount > 0) carCount--; // ลดจำนวนรถ (แต่ต้องไม่ต่ำกว่า 0)
        Serial.println("CAR EXITED SUCCESS!");
        Serial.print("Total Cars: ");
        Serial.println(carCount);
        
        delay(1000);
        gate.write(CLOSE_ANGLE);
        currentState = IDLE;
        delay(1500);
      }
      break;
  }
}
