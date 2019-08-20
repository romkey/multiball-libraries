#include "multiball/wifi.h"

// this stuff is here for the callback and should be moved out
#include "presets.h"
#include "animations.h"
#include "leds.h"
#include "mqtt.h"

#ifdef HAS_BME280
#include "bme280.h"
#endif

static WiFiClient wifi_mqtt_client;
static PubSubClient mqtt_client(wifi_mqtt_client);

bool mqtt_connect() {
  if(mqtt_client.connected())
    return false;

  mqtt_client.connect(uuid, username, password);
  mqtt_client.subscribe(MQTT_CMD_TOPIC);

  mqtt_client.publish("/status", "\"hello world\"");

  return true;
}

bool mqtt_is_connected() {
  return mqtt_client.connected();
}

void mqtt_setup(const char* hostname, uint16_t port, const char* uuid, const char* username, const char* password) {
  mqtt_client.setServer(hostname, port);
  mqtt_client.setCallback(mqtt_callback);
  mqtt_connect();
}

void mqtt_handle() {
  static unsigned long last_mqtt_check = 0;

  mqtt_client.loop();

  if(millis() > last_mqtt_check + 5000) {
    if(mqtt_connect())
      Serial.println("mqtt reconnect");

    last_mqtt_check = millis();
  }
}

void mqtt_publish(const char* topic, const char* payload, bool retain) {
  mqtt_client.publish(topic, payload, retain);
}

void homebus_mqtt_callback(const char*, const char*);

void mqtt_callback(const char* topic, const byte* payload, unsigned int length) {
  char command_buffer[length + 1];

  memcpy(command_buffer, payload, length);
  command[length] = '\0';

  homebus_mqtt_callback(topic, command_buffer);
}
