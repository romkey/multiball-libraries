#include "multiball/wifi.h"
#include "multiball/mqtt.h"

#include <PubSubClient.h>

static WiFiClient wifi_mqtt_client;
static PubSubClient mqtt_client(wifi_mqtt_client);

static char *hostname, *uuid, *username, *password, *subscription = NULL;

bool mqtt_connect();
void mqtt_callback(const char* topic, const byte* payload, unsigned int length);

void mqtt_setup(const char* req_hostname, uint16_t port, const char* req_uuid, const char* req_username, const char* req_password) {
  hostname = (char*)malloc(strlen(req_hostname) + 1);
  strcpy(hostname, req_hostname);

  uuid = (char*)malloc(strlen(req_uuid) + 1);
  strcpy(uuid, req_uuid);

  username = (char*)malloc(strlen(req_username) + 1);
  strcpy(username, req_username);

  password = (char*)malloc(strlen(req_password) + 1);
  strcpy(password, req_password);

  mqtt_client.setServer(hostname, port);
  mqtt_client.setCallback(mqtt_callback);
  mqtt_connect();
}

void mqtt_subscribe(const char* topic) {
  if(subscription)
    free(subscription);

  subscription = (char*)malloc(strlen(topic) + 1);
  strcpy(subscription, topic);
  mqtt_client.subscribe(subscription);
}

bool mqtt_connect() {
  if(mqtt_client.connected())
    return false;

  mqtt_client.connect(uuid, username, password);
  if(subscription)
    mqtt_client.subscribe(subscription);

  return true;
}

bool mqtt_is_connected() {
  return mqtt_client.connected();
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
  command_buffer[length] = '\0';

  homebus_mqtt_callback(topic, command_buffer);
}
