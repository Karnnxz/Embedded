#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "@JumboPlusIoT";
const char* password = "hr92o378";
const char* mqtt_server = "test.mosquitto.org";

#define LDR1 34
#define LDR2 35
#define SERVO_PIN 32
#define BLED 27
#define RLED 26

#define DARK_DETECT 1500   
#define LIGHT_RELEASE 2200 
#define H 3500
#define L 2500
#define OPEN_ANGLE 90
#define CLOSE_ANGLE 0
#define MAX_SLOTS 5

int carCount = 0;
int lastSentCount = -1;
bool isManualMode = false; // เพิ่มตัวแปรเช็กโหมด Manual
enum SystemState { IDLE, CAR_ENTERING, CAR_EXITING, WAIT_CLOSE, MANUAL_HOLD};
SystemState currentState = IDLE;
unsigned long timer = 0;

Servo gate;
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  pinMode(BLED, OUTPUT);
  pinMode(RLED, OUTPUT);

  gate.attach(SERVO_PIN, 500, 2400);
  gate.write(CLOSE_ANGLE);
  Serial.println("Ready. State: IDLE");

  digitalWrite(RLED,HIGH);

  WiFi.begin(ssid, password);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // ทำงานส่วนเซนเซอร์เฉพาะเมื่อไม่ได้อยู่ในโหมด Manual
  if (!isManualMode) {
    int v1 = analogRead(LDR1);
    int v2 = analogRead(LDR2);

    switch (currentState) {
      case IDLE:
        LED(0);
        if (v1 < DARK_DETECT) {
          if (carCount < MAX_SLOTS) {
            gate.write(OPEN_ANGLE); LED(1);
            client.publish("Group7/status", "Entering...", true);
            currentState = CAR_ENTERING;
          } else {
            client.publish("Group7/status", "PARKING FULL!", true);
            digitalWrite(RLED, LOW); delay(200); digitalWrite(RLED, HIGH);
          }
        } 
        else if (v2 < L) {
          LED(1);
          gate.write(OPEN_ANGLE);
          client.publish("Group7/status", "Exiting...", true);
          currentState = CAR_EXITING;
          Serial.println("State: CAR_EXITING");
        }
        break;

      case CAR_ENTERING:
        if (v2 < L) {
          carCount++;
          currentState = WAIT_CLOSE;
          client.publish("Group7/count", String(carCount).c_str(), true);
          timer = millis();
          Serial.println("Success: Car Entered. New Count: " + String(carCount));
        }
        break;

      case CAR_EXITING:
        if (v1 < DARK_DETECT) {
          if(carCount > 0) carCount--;
          currentState = WAIT_CLOSE;
          client.publish("Group7/count", String(carCount).c_str(), true);
          timer = millis();
          Serial.println("Success: Car Exited. New Count: " + String(carCount));
        }
        break;

      case WAIT_CLOSE:
        if (v1 > LIGHT_RELEASE && v2 > H && (millis() - timer > 2000)) {
          gate.write(CLOSE_ANGLE);
          LED(0);
          if (carCount != lastSentCount) {
              lastSentCount = carCount;
          }
          client.publish("Group7/status", "Ready", true);
          currentState = IDLE;
          Serial.println("State: IDLE (Gate Closed)");
        }
        break;

      case MANUAL_HOLD:
        break;
    }
  }
}

void LED(int a){
    if(a==1){
    digitalWrite(BLED,HIGH);
    digitalWrite(RLED,LOW);}
    if(a==0){
    digitalWrite(BLED,LOW);
    digitalWrite(RLED,HIGH);}
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  String strTopic = String(topic);
  
  // ตรวจสอบการสลับโหมดจากปุ่ม Toggle
  if (strTopic == "Group7/mode") {
    if (msg == "MANUAL") {
      isManualMode = true;
      client.publish("Group7/status", "MODE: MANUAL", true);
      Serial.println("Switched to MANUAL MODE");
    } else {
      isManualMode = false;
      currentState = IDLE; // กลับไปเริ่มระบบออโต้ใหม่
      client.publish("Group7/status", "MODE: AUTO (Ready)", true);
      Serial.println("Switched to AUTO MODE");
    }
  }
  
  // ตรวจสอบคำสั่งเปิด-ปิด
  if (strTopic == "Group7/command") {
    if (msg == "OPEN") {
      gate.write(OPEN_ANGLE);
      LED(1);
      if(isManualMode) currentState = MANUAL_HOLD; 
      client.publish("Group7/status", "Manual Opened", true);
    } 
    else if (msg == "CLOSE") {
      gate.write(CLOSE_ANGLE);
      LED(0);
      currentState = IDLE;
      client.publish("Group7/status", "Ready", true);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32_Parking_Final")) {
      client.subscribe("Group7/command");
      client.subscribe("Group7/mode"); // Subscribe หัวข้อโหมดเพิ่ม
    } else { delay(5000); }
  }
}
