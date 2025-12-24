#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

const char* ssid = "Karn_2.4GHz";
const char* password = "Karn2006x";
const char* mqtt_server = "test.mosquitto.org";

const char* topic_count = "Group7/count";
const char* topic_status = "Group7/status";
const char* topic_cmd = "Group7/command";

#define LDR1 34
#define LDR2 35
#define SERVO_PIN 32
#define GREEN_LED 26
#define RED_LED 27

#define DARK_DETECT 1000
#define LIGHT_RELEASE 2000

Servo gate;
WiFiClient espClient;
PubSubClient client(espClient);

int carCount = 0;
enum SystemState { IDLE, CAR_ENTERING, CAR_EXITING, WAIT_CLOSE };
SystemState currentState = IDLE;
unsigned long timer = 0;

void updateLEDs(bool isOpen) {
  if (isOpen) {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  } else {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  gate.attach(SERVO_PIN);
  gate.write(0); 
  updateLEDs(false); // เริ่มต้นที่ไฟแดง

  WiFi.begin(ssid, password);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  if (msg == "OPEN" && currentState == IDLE) {
    gate.write(90); updateLEDs(true);
    currentState = WAIT_CLOSE;
    timer = millis();
  }
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  int v1 = analogRead(LDR1);
  int v2 = analogRead(LDR2);

  switch (currentState) {
    case IDLE:
      if (v1 < DARK_DETECT) {
        gate.write(90); updateLEDs(true);
        client.publish(topic_status, "Entering...", true);
        currentState = CAR_ENTERING;
      } else if (v2 < DARK_DETECT) {
        gate.write(90); updateLEDs(true);
        client.publish(topic_status, "Exiting...", true);
        currentState = CAR_EXITING;
      }
      break;

    case CAR_ENTERING:
      if (v2 < DARK_DETECT) {
        carCount++;
        timer = millis();
        currentState = WAIT_CLOSE;
      }
      break;

    case CAR_EXITING:
      if (v1 < DARK_DETECT) {
        if(carCount > 0) carCount--;
        timer = millis();
        currentState = WAIT_CLOSE;
      }
      break;

    case WAIT_CLOSE:
      if (v1 > LIGHT_RELEASE && v2 > LIGHT_RELEASE && (millis() - timer > 2000)) {
        gate.write(0);
        updateLEDs(false);
        client.publish(topic_count, String(carCount).c_str(), true);
        client.publish(topic_status, "Ready", true);
        currentState = IDLE;
      }
      break;
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32_Parking_System")) {
      client.subscribe(topic_cmd);
      client.publish(topic_status, "Ready", true);
    } else { delay(5000); }
  }
}
