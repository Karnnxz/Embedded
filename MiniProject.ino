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

#define DARK_DETECT 1200   
#define LIGHT_RELEASE 2000 
#define OPEN_ANGLE 90
#define CLOSE_ANGLE 0

Servo gate;
WiFiClient espClient;
PubSubClient client(espClient);

int carCount = 0;
int lastSentCount = -1;
enum SystemState { IDLE, CAR_ENTERING, CAR_EXITING, WAIT_CLOSE };
SystemState currentState = IDLE;
unsigned long timer = 0;

void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) { message += (char)payload[i]; }
  Serial.println("Message arrived [" + String(topic) + "] " + message);
  
  if (String(topic) == topic_cmd && message == "OPEN") {
    if (currentState == IDLE) {
      gate.write(OPEN_ANGLE);
      client.publish(topic_status, "Manual Open", true);
      timer = millis();
      currentState = WAIT_CLOSE;
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    String clientId = "ESP32-Group7-" + String(random(0, 999));
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(topic_cmd);
      client.publish(topic_status, "System Online", true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  gate.attach(SERVO_PIN, 500, 2400);
  gate.write(CLOSE_ANGLE);
  Serial.println("Ready. State: IDLE");
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  int v1 = analogRead(LDR1);
  int v2 = analogRead(LDR2);

  switch (currentState) {
    case IDLE:
      if (v1 < DARK_DETECT) {
        gate.write(OPEN_ANGLE);
        client.publish(topic_status, "Entering...", true);
        currentState = CAR_ENTERING;
        Serial.println("State: CAR_ENTERING");
      } else if (v2 < DARK_DETECT) {
        gate.write(OPEN_ANGLE);
        client.publish(topic_status, "Exiting...", true);
        currentState = CAR_EXITING;
        Serial.println("State: CAR_EXITING");
      }
      break;

    case CAR_ENTERING:
      if (v2 < DARK_DETECT) {
        carCount++;
        currentState = WAIT_CLOSE;
        timer = millis();
        Serial.println("Success: Car Entered. New Count: " + String(carCount));
      }
      break;

    case CAR_EXITING:
      if (v1 < DARK_DETECT) {
        if(carCount > 0) carCount--;
        currentState = WAIT_CLOSE;
        timer = millis();
        Serial.println("Success: Car Exited. New Count: " + String(carCount));
      }
      break;

    case WAIT_CLOSE:
      // รอให้เซนเซอร์ทั้งสองตัวสว่าง (รถพ้นไปแล้วจริง ๆ) และหน่วงเวลาเพิ่มเล็กน้อย
      if (v1 > LIGHT_RELEASE && v2 > LIGHT_RELEASE && (millis() - timer > 2000)) {
        gate.write(CLOSE_ANGLE);
        client.publish(topic_status, "Ready", true);
        
        // ส่งจำนวนรถเฉพาะเมื่อมีการเปลี่ยนแปลง
        if (carCount != lastSentCount) {
            client.publish(topic_count, String(carCount).c_str(), true);
            lastSentCount = carCount;
        }
        
        currentState = IDLE;
        Serial.println("State: IDLE (Gate Closed)");
      }
      break;
  }
}
