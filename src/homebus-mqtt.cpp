#include <multiball/app.h>
#include <multiball/homebus.h>
#include <multiball/mqtt.h>

#ifdef ESP32
#include <HTTPClient.h>
#endif

#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#endif

#include "homebus_config.h"

void homebus_mqtt_override_prefix(const char *prefix) {
  homebus_endpoint = prefix;
  override_prefix = true;
}

void homebus_use_envelope(boolean flag) {
  use_envelope = flag;
}

void homebus_uuid(String new_uuid) {
  device_id = new_uuid;

  if(!override_prefix) {
    homebus_endpoint = "homebus/device/" + new_uuid;
    homebus_cmd_endpoint = "homebus/device/" + new_uuid + "/org.homebus.experimental.command";
  }
}

void homebus_mqtt_setup() {
  static boolean already_setup = false;

  if(already_setup) {
    Serial.println("homebus_mqtt_setup() already set up");
    delay(300);
    return;
  }

  already_setup = true;

  Serial.println("calling mqtt_setup");
  delay(500);
  mqtt_setup(mqtt_broker, mqtt_port, mqtt_username, mqtt_password);

  Serial.println("...done");
  delay(500);

  return;

  if(_consumes[0] == NULL)
    Serial.println("NULL");
  else
    Serial.println(_consumes[0]);

  delay(300);


  for(int i = 0; _consumes[i]; i++) {
    char buf[sizeof("homebus/device/+/") + strlen(_consumes[i]) + 1];
    
    strcpy(buf, "homebus/device/+/");
    strcat(buf, _consumes[i]);

    Serial.printf("Homebus sub %i to %s\n", i, buf);
    delay(500);

    mqtt_subscribe(buf);
  }
}

/*
 * temporary function for transition to new libraries
 */
void homebus_stuff(const char *broker, uint16_t port, const char *username, const char *password, const char *uuid) {
  mqtt_broker = String(broker);
  mqtt_port = port;
  mqtt_username = String(username);
  mqtt_password = String(password);
  device_id = String(uuid);
  homebus_state = HOMEBUS_STATE_SUCCESS;

  Serial.println("about to persist STUFF");
  delay(500);
  homebus_persist();
}

void homebus_publish(const char *msg) {
  if(homebus_state == HOMEBUS_STATE_SUCCESS)
    mqtt_publish(homebus_endpoint.c_str(), msg, true);
}

#define HOMEBUS_ENVELOPE "{ \"source\": \"%s\", \"timestamp\": %lu, \"contents\": { \"ddc\": \"%s\", \"payload\": %s } }"
#define SIZEOF_UUID sizeof("UUUUUUUU-WWWW-XXXX-YYYY-ZZZZZZZZZZZZ")
#define SIZEOF_TIMESTRING sizeof("2148336000")

void homebus_send_to(const char *uuid, const char *ddc, const char *msg) {
  size_t buf_len = sizeof(HOMEBUS_ENVELOPE) + SIZEOF_UUID + strlen(ddc) + strlen(msg) + 1;
  size_t topic_len = sizeof("homebus/device//") + SIZEOF_UUID + strlen(ddc) + 1;

  char buf[buf_len];
  char topic[topic_len];

  if(homebus_state == HOMEBUS_STATE_SUCCESS) {
    
    snprintf(topic, topic_len, "homebus/device/%s/%s", uuid, ddc);
    if(use_envelope) {
      snprintf(buf, buf_len, HOMEBUS_ENVELOPE, device_id.c_str(), time(NULL), ddc, msg);
    } else {
      
    }

    mqtt_publish(topic, buf, true);
  }
}

void homebus_publish_to(const char *ddc, const char *msg) {
  homebus_send_to(device_id.c_str(), ddc, msg);
}

static char *find_payload_start(char *msg, size_t length) {
  char *payload_start = strstr(msg, "payload\":");

  if(payload_start == NULL)
    return NULL;

  payload_start = index(payload_start, '{');
  if(payload_start == NULL)
    return NULL;
  
  
  return payload_start;
}

// this is really oversimplified. it assumes there are no sub-objects in the JSON.
static char *find_payload_end(char *msg, size_t length) {
  return index(msg, '}');
}

// this is a destructive operation that modifies msg
static char *isolate_homebus_payload(char *msg, size_t length) {
  char *payload_start = find_payload_start(msg, length);
  char *payload_end;

  if(payload_start == NULL)
    return NULL;

  payload_end = find_payload_end(payload_start, length - (payload_start - msg));
  if(payload_end == NULL)
    return NULL;

  // here we mutate msg by truncating it at the end of the payload
  payload_end[1] = '\0';

  return payload_start;
}

void homebus_mqtt_callback(const char *topic, char *msg, size_t length) {
  StaticJsonDocument<64> filter;
  StaticJsonDocument<256> doc;

  filter["source"] = true;
  filter["timestamp"] = true;
  filter["contents"]["ddc"] = true;

  DeserializationError error = deserializeJson(doc, (const char*)msg, DeserializationOption::Filter(filter));
#ifdef VERBOSE
  Serial.printf("JSON capacity used %d\n", doc.memoryUsage());
#endif
  if(error) {
    Serial.print("homebus_mqtt_callback: deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  //  const char* source = doc["source"]; // "548de014-0389-438c-9a60-0f92fe37b4d0"
  //  unsigned long timestamp = doc["timestamp"];
  //  const char* ddc = doc["contents"]["ddc"];

#ifdef VERBOSE
  Serial.println("HOMEBUS");
  Serial.printf("\ngot msg %u bytes to topic %s: %.*s\n\n", length, topic, length, msg);
#endif

  char *payload = isolate_homebus_payload(msg, length);
  if(payload == NULL) {
    Serial.println("couldn't isolate Homebus payload");
    return;
  }

#ifdef VERBOSE
  Serial.printf("\nhomebus_mqtt_callback: got source %s ddc %s timestamp %lu payload %s\n\n", source, ddc, timestamp, payload);
#endif

}
