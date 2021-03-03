#ifdef ESP8266

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
#ifdef VERBOSE
    Serial.println("Connected to MQTT.");
    Serial.print("Session present: ");
    Serial.println(sessionPresent);
#endif

    for(auto item = subscriptions.cbegin(); item != subscriptions.cend(); item++)
      mqtt_client.subscribe(*item, 0);
}

static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
#ifdef VERBOSE
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
#endif
 
  if(WiFi.isConnected())
    mqtt_reconnect_timer.once(30, mqtt_connect);
}

void onMqttMessage(const char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
#ifdef VERBOSE
  Serial.printf("\n\nonMqttMessage(topic: %s, length: %u, index: %u, total: %u, payload: %.*s\n\n", topic, len, index, total, len, payload);
#endif

  if(index != 0 || total != len) {
#ifdef VERBOSE
    Serial.println("punting on incomplete msg");
#endif
    return;
  }

  void mqtt_callback(const char*, char*, size_t);
  mqtt_callback(topic, payload, len);
}

void mqtt_setup(String req_hostname, uint16_t req_port, String req_username, String req_password) {
  mqtt_client.onConnect(onMqttConnect);
  mqtt_client.onDisconnect(onMqttDisconnect);
  mqtt_client.onMessage(onMqttMessage);

  hostname = strdup(req_hostname.c_str());
  username = strdup(req_username.c_str());
  password = strdup(req_password.c_str());

  port = req_port;

  mqtt_client.setClientId(username);
  mqtt_client.setCredentials(username, password);
  mqtt_client.setServer(hostname, port);
  
#ifdef VERBOSE
  Serial.println("MQTT connecting");
#endif
  mqtt_connect();
}

void mqtt_subscribe(const char* topic) {
  char* subscription;

#ifdef VERBOSE
  Serial.printf("mqtt_subscribe %s\n", topic);
#endif

  subscription = strdup(topic);
  subscriptions.push_back(subscription);

  if(mqtt_client.connected())
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

void homebus_mqtt_callback(const char*, char*, size_t);

void mqtt_callback(const char* topic, char* payload, size_t length) {
  char command_buffer[length + 1];

  memcpy(command_buffer, payload, length);
  command_buffer[length] = '\0';

  homebus_mqtt_callback(topic, command_buffer, length);
}

#endif // ESP8266
