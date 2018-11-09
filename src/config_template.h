// Rename this file to config.h and change accordingly

#include <Arduino.h>

// WiFi
#define WIFI_SSID ""
#define WIFI_PASS ""

// MQTT
#define MQTT_HOST IPAddress(192, 168, 1, 2)
#define MQTT_PORT 1883
#define MQTT_CLIENT "esp8266-sensors"

// SENSORS
#define SENSOR_POLL_RATE 10 // seconds
#define SENSOR_DHT_PIN D1
