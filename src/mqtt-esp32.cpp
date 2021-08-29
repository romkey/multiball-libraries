#ifdef ESP32

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
    Serial.println("MQTT connected");

    _mqtt_is_connected = true;
#if 0
    for(auto item = subscriptions.cbegin(); item != subscriptions.cend(); item++) {
      Serial.print("subscribe to ");
      Serial.println(*item);
      esp_mqtt_client_subscribe(client, *item, 0);
    }
#endif
    break;

  case MQTT_EVENT_DATA: {
    Serial.println("MQTT EVENT DATA");
    break;

    delay(500);

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
    Serial.println("MQTT disconnected");

    _mqtt_is_connected = false;

    //    if(WiFi.isConnected())
    // mqtt_reconnect_timer.once(30, mqtt_connect);

    break;

  case MQTT_EVENT_ERROR:
    Serial.println("MQTT error");
    break;
  case MQTT_EVENT_SUBSCRIBED:
    Serial.println("MQTT subscribed");
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    Serial.println("MQTT unsubscribed");
    break;
  case MQTT_EVENT_PUBLISHED:
    Serial.println("MQTT event published");
    break;
  case MQTT_EVENT_BEFORE_CONNECT:
    Serial.println("MQTT event before connect");
    break;
  default:
    Serial.println("MQTT unknown event");
    break;
    return 0;
  }

  return 0;
}


void mqtt_setup(String req_hostname, uint16_t req_port, String req_username, String req_password) {
  Serial.println("mqtt_setup entered");
  delay(300);

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

void mqtt_callback(const char* topic, const byte* payload, unsigned int length) {
  char command_buffer[length + 1];

  memcpy(command_buffer, payload, length);
  command_buffer[length] = '\0';

  homebus_mqtt_callback((const char*)topic, (char*)command_buffer, length);
}

#endif
