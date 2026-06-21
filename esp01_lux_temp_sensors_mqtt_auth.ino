// Remember to change WIFI_SSID and WIFI_PASSWORD, MQTT Broker settings and MQTT Topics to suit your needs.
// It would be more correct to store these in a separate .secrets file

// Here both sensors, luxmeter and temp sensor are both connected to same I2C bus. Check wether your modules share same addresses as in the code
// Topics are sent to MQTT broker from where they are easily read wherever.

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <Wire.h>
#include "BH1750FVI.h"
#include <AHT20.h>

#define WIRE Wire
#define WIFI_SSID "your_ssid"
#define WIFI_PASSWORD "your_pw"

BH1750FVI luxMeter(0x23);
AHT20 aht20;

//Mosquitto MQTT Broker
#define MQTT_HOST IPAddress(192, 168, 1, 11)
#define MQTT_PORT 1883
#define MQTT_USER "mqtt_user"
#define MQTT_USER_PW "mqtt_user_pw"

// Temperature MQTT Topics
#define MQTT_PUB_TEMP "temp3"
#define MQTT_PUB_HUM "hum3"
#define MQTT_PUB_LUX "lux1"

float lux;   
float temp;  
float hum;   

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;         // Stores last time temperature was published
const long interval = 20000;               // Interval at which to publish sensor readings
unsigned long previousMillisSensors = 0;  // Stores last time temperature was published
const long intervalSensors = 15000;       // Interval at which to publish sensor readings

void readSensors() {
  unsigned long currentMillisSensors = millis();
  // Every X number of seconds (interval = 10 seconds)
  // it publishes a new MQTT message
  if (currentMillisSensors - previousMillisSensors >= intervalSensors) {
    // Save the last time a new reading was published
    previousMillisSensors = currentMillisSensors;
    lux = luxMeter.getLux();
    temp = aht20.getTemperature();
    hum = aht20.getHumidity();
  }
}

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach();  // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.setCredentials(MQTT_USER, MQTT_USER_PW);
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

/*void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}*/

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  WIRE.begin(2, 0);  //esp01 pinnit
  //valoanturin alustus
  luxMeter.powerOn();
  luxMeter.setContHighRes();
  //lämpö/kosteusanturin alustus
  aht20.begin();

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqttClient.onSubscribe(onMqttSubscribe);
  //mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  connectToWifi();
}

void loop() {

  readSensors();

  unsigned long currentMillis = millis();
  // Every X number of seconds (interval = 10 seconds)
  // it publishes a new MQTT message
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;



    // Publish an MQTT message on topic MQTT_PUB_TEMP
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(temp).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_TEMP, packetIdPub1);
    Serial.printf("Message: %.2f \n", temp);

    // Publish an MQTT message on   MQTT_PUB_HUM
    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(hum).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_HUM, packetIdPub2);
    Serial.printf("Message: %.2f \n", hum);

    // Publish an MQTT message on topic  MQTT_PUB_LUX
    uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_LUX, 1, true, String(lux).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_LUX, packetIdPub3);
    Serial.printf("Message: %.2f \n", lux);
  }
}
