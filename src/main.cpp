#include "config.h"
#include <Arduino.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include <DHTesp.h>
#include <OneWire.h>
#include <DallasTemperature.h>

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

DHTesp dht;
Ticker dhtTimer;

OneWire oneWire(SENSOR_DS_PIN);
DallasTemperature dsSensors(&oneWire);
DeviceAddress dsAddress;
Ticker dsTimer;

void connectToWifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.setClientId(MQTT_CLIENT);
  mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.print("Connected to Wi-Fi. IP: ");
  Serial.println(WiFi.localIP());
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
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

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
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
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
}

void onMqttPublish(uint16_t packetId) {
  // Serial.println("Publish acknowledged.");
  // Serial.print("  packetId: ");
  // Serial.println(packetId);
}

void readDHT() {
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  Serial.print("Status: ");
  Serial.print(dht.getStatusString());
  Serial.print(", Temperature: ");
  Serial.print(temperature);
  Serial.print("°C, Humidity: ");
  Serial.print(humidity);
  Serial.println("%");

  char temp[] = "";
  dtostrf(temperature, 3, 2, temp);
  mqttClient.publish("sensor/dht22/temperature", 2, true, temp);
  char hum[] = "";
  dtostrf(humidity, 3, 2, hum);
  mqttClient.publish("sensor/dht22/humidity", 2, true, hum);
}

void readDallas() {
  dsSensors.requestTemperaturesByAddress(dsAddress);
  float temperature = dsSensors.getTempC(dsAddress);
  Serial.print("Dallas: ");
  Serial.print(temperature);
  Serial.println("°C");

  char temp[] = "";
  dtostrf(temperature, 3, 2, temp);
  mqttClient.publish("sensor/ds18b20/temperature", 2, true, temp);
}

void setupSensors() {
  // DHT
  dht.setup(SENSOR_DHT_PIN, DHTesp::DHT22);
  dhtTimer.attach(SENSOR_POLL_RATE, readDHT);

  delay(1000);

  // Dallas
  dsSensors.begin();
  if (dsSensors.getAddress(dsAddress, 0)) {
    dsTimer.attach(SENSOR_POLL_RATE, readDallas);
  } else {
    Serial.print("No Dallas sensor connected");
  }
}

void setup() {
  Serial.begin(9600);

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();
  setupSensors();
}

void loop() {

} 
