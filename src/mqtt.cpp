#include <vector>

#include "multiball/app.h"
#include "multiball/wifi.h"
#include "multiball/mqtt.h"
#include "multiball/homebus.h"

#include <Ticker.h>
#include <AsyncMqttClient.h>

static AsyncMqttClient mqtt_client;

static char *hostname, *username, *password;
static uint16_t port = 1883;

static Ticker mqtt_reconnect_timer;

static std::vector<char*> subscriptions;

void mqtt_connect();
void mqtt_callback(const char* topic, const byte* payload, unsigned int length);

static void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);

  for(auto item = subscriptions.cbegin(); item != subscriptions.cend(); item++)
    mqtt_client.subscribe(*item, 0);
}

static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if(reason == AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT)
    Serial.println("Bad server fingerprint.");

  if(reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED)
    Serial.println("TCP disconnect");

  if(reason == AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION)
    Serial.println("MQTT unacceptable protocol");

  if(reason == AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED)
    Serial.println("MQTT identifier rejected");

  if(reason == AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS)
    Serial.println("MQTT malformed credentials");

  if(reason == AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED)
    Serial.println("MQTT not authorized");

  if(reason == AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE)
    Serial.println("not enough space");
 
  if(WiFi.isConnected())
    mqtt_reconnect_timer.once(30, mqtt_connect);
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

void mqtt_setup(String req_hostname, uint16_t req_port, String req_username, String req_password) {
  mqtt_client.onConnect(onMqttConnect);
  mqtt_client.onDisconnect(onMqttDisconnect);
  mqtt_client.onMessage(onMqttMessage);

  hostname = (char*)malloc(req_hostname.length() + 1);
  strcpy(hostname, req_hostname.c_str());

  username = (char*)malloc(req_username.length() + 1);
  strcpy(username, req_username.c_str());

  password = (char*)malloc(req_password.length() + 1);
  strcpy(password, req_password.c_str());

  port = req_port;

  mqtt_client.setCredentials(username, password);
  mqtt_client.setServer(hostname, port);
  
  Serial.println("MQTT connecting");
  mqtt_connect();
}

void mqtt_subscribe(const char* topic) {
  char* subscription;

  Serial.printf("mqtt_subscribe %s\n", topic);

  subscription = (char*)malloc(strlen(topic) + 1);
  strcpy(subscription, topic);
  subscriptions.push_back(subscription);
  mqtt_client.subscribe(subscription, 0);
}

void mqtt_connect() {
  if(mqtt_client.connected())
    return;

  mqtt_client.connect();
}

bool mqtt_is_connected() {
  return mqtt_client.connected();
}

void mqtt_publish(const char* topic, const char* payload, bool retain) {
  mqtt_client.publish(topic, 0, retain, payload);
}

void homebus_mqtt_callback(const char*, char*);

void mqtt_callback(const char* topic, const byte* payload, unsigned int length) {
  char command_buffer[length + 1];

  memcpy(command_buffer, payload, length);
  command_buffer[length] = '\0';

  homebus_callback(topic, command_buffer);
}
