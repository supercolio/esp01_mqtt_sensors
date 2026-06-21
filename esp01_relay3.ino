// This code assumes you're using a generic esp-01 relay board.
// Change ssid, pw, broker address and user below, as well as the topics to suit your needs.
// outTopic = sensor broadcasts its status, wether the relay is on or off
// inTopic = send values 0 or 1 to the module to turn it on or off.


#include <WiFi.h>
#include <PubSubClient.h>

const int relayPin = 0;
int relayPosition;
#define RELAY_ON LOW  //for some reason the logic is inverted on the module, pin low = relay on. Here we revert the logic.
#define RELAY_OFF HIGH

unsigned long lastMsg = 0;

const char* ssid = "your_ssid";             // Change this to your WiFi SSID
const char* password = "your_pw";    // Change this to your WiFi password
const char* mqtt_server = "192.168.1.11";  // mqtt broker address
uint16_t mqtt_port = 1883;                 // broker port
const char* user = "mqtt_user";             //broker user name
const char* userpw = "mqtt_user_pw";         //broker user pw

WiFiClient espClient;
PubSubClient client(espClient);

const char* outTopic = "relay2/status";  
const char* inTopic = "relay2/set";      

bool mqttConnected = false;      // track state
bool lastMqttConnected = false;  // previous state

void setup() {
  pinMode(relayPin, OUTPUT);  // Initialize the relayPin pin as an output
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }

  // Blink relay to show WiFi status
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(relayPin, RELAY_ON);  // WiFi OK
    delay(500);
    digitalWrite(relayPin, RELAY_OFF);
  }

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  digitalWrite(relayPin, RELAY_OFF);
  relayPosition = RELAY_OFF;
}

void loop() {
  if (!client.connected()) {
    mqttConnected = false;
    reconnectMQTT();
  } else {
    mqttConnected = true;
  }

  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    if (relayPosition == RELAY_ON) {
      client.publish(outTopic, "on");
    }
    if (relayPosition == RELAY_OFF) {
      client.publish(outTopic, "off");
    }
  }
}

void callback(char* inTopic, byte* payload, unsigned int length) {
  // handle message arrived
  if ((char)payload[0] == '0') {        // Switch off the relay if an 0 was received as first character
    digitalWrite(relayPin, RELAY_OFF);  // HIGH = off , LOW = on
    relayPosition = RELAY_OFF;
  }
  if ((char)payload[0] == '1') {  // Switch on the relay if an 1 was received as first character
    digitalWrite(relayPin, RELAY_ON);
    relayPosition = RELAY_ON;
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    // Serial.println("Reconnecting to MQTT");
    // Optionally, use a unique client ID
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), user, userpw)) {
      // Serial.println("Connected!");
      client.publish(outTopic, "Esp01 connected");
      client.subscribe(inTopic);

    } else {
      /*
      Serial.println("Failed, rc=");
      Serial.println(client.state());

      int state = client.state();
      if (state == -1) {

        Serial.println("Connection failed");
      } else if (state == -2) {
        Serial.println("Connection lost");
      } else if (state == -3) {
        Serial.println("Connection lost");
      } else if (state == -4) {
        Serial.println("Server unavailable");
      } else if (state == -5) {
        Serial.println("Bad protocol");
      } else if (state == -6) {
        Serial.println("Bad client id");
      } else if (state == -7) {
        Serial.println("Bad credentials");
      } else if (state == -8) {
        Serial.println("Unauthorized");
      }

      delay(2000);
      Serial.println("Retrying in 2sec    ");
      */
      delay(2000);
    }
  }
}