#include <vector>

#include "multiball/app.h"
#include "multiball/wifi.h"
#include "multiball/mqtt.h"
#include "multiball/homebus.h"

#include <Ticker.h>

#include "esp_event.h"
#include "mqtt_client.h"

static esp_mqtt_client_config_t mqtt_cfg;
static esp_mqtt_client_handle_t client;

static boolean _mqtt_is_connected = false;

static Ticker mqtt_reconnect_timer;

static std::vector<char*> subscriptions;

void mqtt_connect();
void mqtt_callback(const char* topic, const byte* payload, unsigned int length);

static int mqtt_event_handler(esp_mqtt_event_t *e) { 
  Serial.printf("MQTT event id %d\n", e->event_id);

  switch(e->event_id) {
  case MQTT_EVENT_CONNECTED:
    Serial.println("mqtt connected");

    _mqtt_is_connected = true;

    for(auto item = subscriptions.cbegin(); item != subscriptions.cend(); item++)
      esp_mqtt_client_subscribe(client, *item, 0);

    break;

  case MQTT_EVENT_DATA: {
    char topic[e->topic_len + 1];
    char data[e->data_len + 1];

    memcpy(topic, e->topic, e->topic_len);
    memcpy(data, e->data, e->data_len);

    topic[e->topic_len] = '\0';
    data[e->data_len] = '\0';

    Serial.printf("got %d bytes to %s\n", e->data_len, topic);
    Serial.println(data);

    mqtt_callback(topic, (const byte *)data, e->data_len);

    break;
  }

  case MQTT_EVENT_DISCONNECTED:
    _mqtt_is_connected = false;

    if(WiFi.isConnected())
      mqtt_reconnect_timer.once(30, mqtt_connect);

    break;

  case MQTT_EVENT_ERROR:
  case MQTT_EVENT_SUBSCRIBED:
  case MQTT_EVENT_UNSUBSCRIBED:
  case MQTT_EVENT_PUBLISHED:
  case MQTT_EVENT_BEFORE_CONNECT:
  default:
    return 0;
  }


  return 0;
}


void mqtt_setup(String req_hostname, uint16_t req_port, String req_username, String req_password) {
  //    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);

  mqtt_cfg.host = strdup(req_hostname.c_str());
  mqtt_cfg.username = strdup(req_username.c_str());
  mqtt_cfg.password = strdup(req_password.c_str());
  mqtt_cfg.port = req_port;

  mqtt_cfg.event_handle = mqtt_event_handler;

  client = esp_mqtt_client_init(&mqtt_cfg);

  Serial.println("MQTT connecting");
  mqtt_connect();
}

void mqtt_subscribe(const char* topic) {
  char* subscription;

  Serial.printf("mqtt_subscribe %s\n", topic);

  subscription = strdup(topic);
  subscriptions.push_back(subscription);

  if(mqtt_is_connected())
    esp_mqtt_client_subscribe(client, subscription, 0);
}

void mqtt_connect() {
  if(mqtt_is_connected())
    return;

  esp_mqtt_client_start(client);
}

bool mqtt_is_connected() {
  return _mqtt_is_connected;
}

void mqtt_publish(const char* topic, const char* payload, bool retain) {
  esp_mqtt_client_publish(client, topic, payload, strlen(payload), 0, 1);
}

void homebus_mqtt_callback(const char*, char*);

void mqtt_callback(const char* topic, const byte* payload, unsigned int length) {
  char command_buffer[length + 1];

  memcpy(command_buffer, payload, length);
  command_buffer[length] = '\0';

  homebus_mqtt_callback((const char*)topic, (const char*)command_buffer);
}
