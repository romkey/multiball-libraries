#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <Esp.h>
#include <WiFi.h>
#endif

#include "config.h"

#ifdef USE_MQTT

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

  mqtt_client.connect(MQTT_UUID, MQTT_USER, MQTT_PASS);
  mqtt_client.subscribe(MQTT_CMD_TOPIC);

  mqtt_client.publish("/status", "\"hello world\"");

  return true;
}

void mqtt_setup() {
  mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
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

void mqtt_callback(const char* topic, const byte* payload, unsigned int length) {
  char command_buffer[length + 1];
  char* command = command_buffer;

  memcpy(command_buffer, payload, length);
  command[length] = '\0';

  // command is meant to be a valid json string, so get rid of the quotes
  if(command[0] == '"' && command[length-1] == '"') {
    command[length-1] = '\0';
    command += 1;
  }

  char buffer[length + 30];
  snprintf(buffer, length+30, "{ \"cmd\": \"%s\" }", command);

  Serial.printf("command %s\n", command);

  if(strcmp(command, "restart") == 0) {
    ESP.restart();
  }

  if(strcmp(command, "off") == 0) {
    leds_off();
    return;
  }

  if(strcmp(command, "stop") == 0) {
    animation_stop();
    return;
  }

  if(strncmp(command, "speed ", 6) == 0) {
    Serial.printf("speed %s\n", &command[6]);
    animation_speed(String(&command[6]).toFloat());
    return;
  }

  if(strncmp(command, "bright ", 7) == 0) {
    leds_brightness(atoi(&command[7]));
    return;
  }

  if(strncmp(command, "rgb ", 4) == 0) {
    char temp[3] = "xx";
    uint8_t red, green, blue;

    Serial.printf("RGB %s\n", &command[4]);
    temp[0] = command[4];
    temp[1] = command[5];
    red = strtol(temp, 0, 16);

    temp[0] = command[6];
    temp[1] = command[7];
    green = strtol(temp, 0, 16);

    temp[0] = command[8];
    temp[1] = command[9];
    blue = strtol(temp, 0, 16);

    Serial.printf("rgb red %u, green %u, blue %u\n", red, green, blue);
    preset_rgb(red, green, blue);
  }

  if(strncmp(command, "preset ", 7) == 0) {
    Serial.println("got preset");
    Serial.println(&command[7]);

    if(preset_set(&command[7]))
      return;

    mqtt_client.publish("/leds/$error", command);
    return;
  }

  if(strncmp(command, "animation ", 10) == 0) {
    Serial.println("got animation");
    Serial.println(&command[10]);

    if(animation_set(&command[10]))
      return;

    mqtt_client.publish("/leds/$error", command);
    return;
  }
}

#endif
