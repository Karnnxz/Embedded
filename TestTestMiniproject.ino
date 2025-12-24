#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

const char* ssid = "Karn_2.4GHz";
const char* password = "Karn2006x";
const char* mqtt_server = "test.mosquitto.org";

#define LDR1 34
#define LDR2 35
#define SERVO_PIN 32
#define GREEN_LED 26
#define RED_LED 27

const int MAX_SLOTS = 5;
const int OPEN_ANGLE = 90;
const int CLOSE_ANGLE = 0;
#define DARK_DETECT 1000
#define LIGHT_RELEASE 2000

Servo gate;
WiFiClient espClient;
PubSubClient client(espClient);

int carCount = 0;
enum SystemState { IDLE, CAR_ENTERING, CAR_EXITING, WAIT_CLOSE, MANUAL_HOLD };
SystemState currentState = IDLE;
unsigned long timer = 0;

void updateLEDs(bool isOpen) {
  digitalWrite(GREEN_LED, isOpen ? HIGH : LOW);
  digitalWrite(RED_LED, isOpen ? LOW : HIGH);
}

// ฟังก์ชันรับคำสั่งจาก Dashboard
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  
  if (msg == "OPEN") {
    gate.write(OPEN_ANGLE);
    updateLEDs(true);
    currentState = MANUAL_HOLD; // เข้าสู่โหมดค้างไว้จนกว่าจะสั่งปิด
    client.publish("Group7/status", "Manual Opened", true);
  } 
  else if (msg == "CLOSE") {
    gate.write(CLOSE_ANGLE);
    updateLEDs(false);
    currentState = IDLE;
    client.publish("Group7/status", "Ready", true);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  
  gate.attach(SERVO_PIN);
  gate.write(CLOSE_ANGLE);
  updateLEDs(false);

  WiFi.begin(ssid, password);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  int v1 = analogRead(LDR1);
  int v2 = analogRead(LDR2);

  // --- Logic หลัก ---
  switch (currentState) {
    case IDLE:
      // ขาเข้า: เช็กก่อนว่าที่จอดเต็มไหม
      if (v1 < DARK_DETECT) {
        if (carCount < MAX_SLOTS) {
          gate.write(OPEN_ANGLE); updateLEDs(true);
          client.publish("Group7/status", "Entering...", true);
          currentState = CAR_ENTERING;
        } else {
          client.publish("Group7/status", "PARKING FULL!", true);
          // กระพริบไฟแดงเตือนว่าเต็ม
          digitalWrite(RED_LED, LOW); delay(200); digitalWrite(RED_LED, HIGH);
        }
      } 
      // ขาออก: ออกได้เสมอ
      else if (v2 < DARK_DETECT) {
        gate.write(OPEN_ANGLE); updateLEDs(true);
        client.publish("Group7/status", "Exiting...", true);
        currentState = CAR_EXITING;
      }
      break;

    case CAR_ENTERING:
      if (v2 < DARK_DETECT) {
        carCount++;
        client.publish("Group7/count", String(carCount).c_str(), true);
        timer = millis();
        currentState = WAIT_CLOSE;
      }
      break;

    case CAR_EXITING:
      if (v1 < DARK_DETECT) {
        if(carCount > 0) carCount--;
        client.publish("Group7/count", String(carCount).c_str(), true);
        timer = millis();
        currentState = WAIT_CLOSE;
      }
      break;

    case WAIT_CLOSE:
      if (v1 > LIGHT_RELEASE && v2 > LIGHT_RELEASE && (millis() - timer > 2000)) {
        gate.write(CLOSE_ANGLE);
        updateLEDs(false);
        client.publish("Group7/status", "Ready", true);
        currentState = IDLE;
      }
      break;

    case MANUAL_HOLD:
      // อยู่สถานะนี้เฉยๆ จนกว่าจะมีคำสั่ง CLOSE จาก Callback
      break;
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32_Parking_Final")) {
      client.subscribe("Group7/command");
    } else { delay(5000); }
  }
}
