#include <multiball/app.h>
#include <multiball/homebus.h>
#include <multiball/mqtt.h>

#ifdef ESP32
#include <ESPmDNS.h>
#include <HTTPClient.h>
#endif

#ifdef ESP8266
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#endif

static String UUID;
static String refresh_token;
static String mqtt_username, mqtt_password, mqtt_broker;
static uint16_t mqtt_port = 0;

static String homebus_server;

const char *homebus_uuid() {
  return UUID.c_str();
}

const char *homebus_mqtt_username() {
  return mqtt_username.c_str();
}

const char *homebus_mqtt_host() {
  return mqtt_broker.c_str();
}

uint16_t homebus_mqtt_port() {
  return mqtt_port;
}

static boolean override_prefix = false;
static boolean use_envelope = true;
static String homebus_endpoint;
static String homebus_cmd_endpoint;
static boolean provisioned = false;

static homebus_state_t homebus_state = HOMEBUS_STATE_NOT_SETUP;

static unsigned long homebus_next_provision_retry = 0;
static unsigned homebus_provision_retry_time = 60;
#if 0
static unsigned discover_retry_time = 60;
#endif

static void homebus_provision();
static void homebus_provision_request(char*buf, size_t);
static void homebus_process_response(String payload);

static const char *_manufacturer = "";
static const char *_model = "";
static const char *_serial_number = "";
static const char *_pin = "";
static const char **_wo_ddcs, **_ro_ddcs;

static String _homebus_server, _homebus_auth_token;

// friendly name, friendly location, manufacturer, model, update_frequency
void homebus_configure(const char *manufacturer, const char *model, const char *serial_number, const char *pin, const char *write_only_ddcs[], const char *read_only_ddcs[]) {
  _manufacturer = manufacturer;
  _model = model;
  _serial_number = serial_number;
  _pin = pin;

  _wo_ddcs = write_only_ddcs;
  _ro_ddcs = read_only_ddcs;
}

void homebus_set_provisioner(const char *server, const char *auth_token) {
  _homebus_server = String(server);
  _homebus_auth_token = String(auth_token);
}

void homebus_mqtt_override_prefix(const char *prefix) {
  homebus_endpoint = prefix;
  override_prefix = true;
}

void homebus_use_envelope(boolean flag) {
  use_envelope = flag;
}

void homebus_uuid(String new_uuid) {
  UUID = new_uuid;

  if(!override_prefix) {
    homebus_endpoint = "homebus/device/" + UUID;
    homebus_cmd_endpoint = "homebus/device/" + UUID + "/org.homebus.experimental.command";
  }
}

void homebus_mqtt_setup() {
  mqtt_setup(mqtt_broker, mqtt_port, mqtt_username, mqtt_password);

  for(int i = 0; _ro_ddcs[i]; i++) {
    char buf[sizeof("homebus/device/+/") + strlen(_ro_ddcs[i]) + 1];
    strcpy(buf, "homebus/device/+/");
    strcat(buf, _ro_ddcs[i]);

    Serial.printf("Homebus sub to %s\n", buf);

    mqtt_subscribe(buf);
  }
}

void homebus_setup() {
  homebus_restore();

  if(homebus_state == HOMEBUS_STATE_OKAY) {
    homebus_mqtt_setup();
    mqtt_connect();
    return;
  }

#ifdef ESP32
  IPAddress homebus_ip = MDNS.queryHost("homebus.local");
  Serial.print("homebus IP is ");
  Serial.println(homebus_ip);
#endif

  if(!provisioned) {
    homebus_state = HOMEBUS_STATE_PROVISIONING;
    homebus_provision();
  }
}

void homebus_handle() {
  if(homebus_state == HOMEBUS_STATE_PROVISION_WAIT) {
    if(homebus_next_provision_retry < millis()) {
      Serial.println("homebus provision retrying...");
      homebus_next_provision_retry = millis() + homebus_provision_retry_time*1000;
      homebus_provision();
    }
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
  UUID = String(uuid);
  homebus_state = HOMEBUS_STATE_OKAY;

  Serial.println("about to persist STUFF");
  delay(500);
  homebus_persist();
}

// labels must be 15 characters or fewer, so use "hb-" as a prefix, not "homebus-"
void homebus_persist() {
  App.config.set("hb-state", String(homebus_state));
  App.config.set("hb-uuid", UUID.c_str());
  App.config.set("hb-refresh-token", refresh_token.c_str());
  App.config.set("hb-broker", mqtt_broker.c_str());
  App.config.set("hb-username", mqtt_username.c_str());
  App.config.set("hb-password", mqtt_password.c_str());
  App.config.set("hb-port", String(mqtt_port).c_str());
}

void homebus_reset() {
  App.config.clear("hb-state");
  App.config.clear("hb-uuid");
  App.config.clear("hb-refresh-token");
  App.config.clear("hb-broker");
  App.config.clear("hb-username");
  App.config.clear("hb-password");
  App.config.clear("hb-port");
}


void homebus_restore() {
  boolean success;
  String temp;

  temp = App.config.get("hb-state", &success);
  if(success)
    homebus_state = static_cast<homebus_state_t>(temp.toInt());

  temp = App.config.get("hb-uuid", &success);
  if(success)
    homebus_uuid(temp);
  else
    Serial.println("UUID fail");

  temp = App.config.get("hb-refresh-token", &success);
  if(success)
    refresh_token = temp;
  else
    Serial.println("Refresh token fail");

  temp = App.config.get("hb-broker", &success);
  if(success)
    mqtt_broker = temp;
  else
    Serial.println("mqtt_broker fail");

  temp = App.config.get("hb-username", &success);
  if(success)
    mqtt_username = temp;
  else
    Serial.println("mqtt_username fail");

  temp = App.config.get("hb-password", &success);
  if(success)
    mqtt_password = temp;
  else
    Serial.println("mqtt_password fail");

  temp = App.config.get("hb-port", &success);
  if(success)
    mqtt_port = temp.toInt();
  else
    Serial.println("port fail");

  if(mqtt_username == "")
    homebus_state = HOMEBUS_STATE_NOT_SETUP;

  Serial.printf("homebus restore state %d broker %s port %u user %s pass %s uuid %s\n",
		static_cast<int>(homebus_state),
		mqtt_broker.c_str(),
		mqtt_port,
		mqtt_username.c_str(),
		mqtt_password.c_str(),
		UUID.c_str());

  //  mqtt_port = 4567;
  //  homebus_persist();
}

/*
 * generate C++ code at https://arduinojson.org/v6/assistant/
 *
 *

     "identity": {
        "manufacturer": "HomeBus",
        "model": "v0",
        "serial_number": "7",
        "pin": ""
        },
       "ddcs": {
        "write-only": [ "org.homebus.experimental.thermostat" ],
        "read-only": [ "org.homebus.experimental.air-sensor" ],
        "read-write": [ ]
    }
  }

 */

static void homebus_provision_request(char *buf, size_t buf_length) {
  const size_t capacity = JSON_ARRAY_SIZE(1) + 2*JSON_ARRAY_SIZE(15) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4);
  StaticJsonDocument<capacity> doc;

  JsonObject provision = doc.createNestedObject("provision");

  JsonObject provision_identity = provision.createNestedObject("identity");
  provision_identity["manufacturer"] = _manufacturer;
  provision_identity["model"] = _model;
  provision_identity["serial_number"] = _serial_number;
  provision_identity["pin"] = _pin;

  JsonObject provision_ddcs = provision.createNestedObject("ddcs");

  JsonArray provision_ddcs_write_only = provision_ddcs.createNestedArray("write-only");

  const char **ptr = _wo_ddcs;
  while(1) {
    if(*ptr == NULL)
      break;

    Serial.printf("adding %s to WO ddcs\n", *ptr);
    provision_ddcs_write_only.add(*ptr);
    ptr++;
  }

  ptr = _ro_ddcs;
  JsonArray provision_ddcs_read_only = provision_ddcs.createNestedArray("read-only");

  while(1) {
    if(*ptr == NULL)
      break;

    Serial.printf("adding %s to RO ddcs\n", *ptr);
    provision_ddcs_read_only.add(*ptr);
    ptr++;
  }

  serializeJsonPretty(doc, buf, buf_length);
  Serial.println(strlen(buf));
  Serial.println(buf);

  serializeJson(doc, buf, buf_length);
  Serial.println(strlen(buf));
  Serial.println(buf);
}

void homebus_receive(const char *topic, char *msg, size_t length) {
}

void homebus_system(JsonObject system) {
  system["name"] = App.hostname();
  system["build"] = App.build_info();
  system["freeheap"] = ESP.getFreeHeap();
  system["uptime"] = App.uptime();
  system["ip"] = App.ip_address();
  system["mac_address"] = App.mac_address();
  system["rssi"] = WiFi.RSSI();
  system["reboots"] = App.boot_count();
  system["wifi_failures"] = App.wifi_failures();
}

/*
   from https://arduinojson.org/v6/assistant/

   {
     "status": "retry",
     "retry_time": 60
     }

   {
     "status": "success",
     "url": "http://some-long-name.homebus.io:80",
     "server": "some-long-name.homebus.io",
     "port": 80,
     "secure": false
     }

*/
#if 0
static void homebus_process_discover(String payload) {
  const size_t capacity = JSON_OBJECT_SIZE(5) + 110;
  StaticJsonDocument<capacity> doc;

  deserializeJson(doc, payload);

  if(strcmp(doc["status"], "retry") == 0) {
    discover_retry_time = doc["retry_time"];
    return;
  }

  if(strcmp(doc["status"], "success") == 0)
     return;

  const char *url = doc["url"]; // "http://some-long-name.homebus.io:80"
  const char *server = doc["server"]; // "some-long-name.homebus.io"
  int port = doc["port"]; // 80
  bool secure = doc["secure"]; // false
}

static void homebus_discover() {
  HTTPClient http;

  http.begin("http://discover.homebus.io/discover");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  
  int httpCode = http.POST("");
  if(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
    String payload = http.getString();
    Serial.println("OKAY!");
    Serial.println(payload);

    homebus_process_discover(payload);
  } else {
    Serial.print("httpCode is ");
    Serial.println(httpCode);
  }

  http.end();
}
#endif

static void homebus_provision() {
  char buf[1024];
  HTTPClient http;

  homebus_provision_request(buf, 1024);

  //  http.begin("http://hipster.homebus.io/provision");
  //  http.begin("http://ctrlh.homebus.io:5678/provision");

  http.begin(String("http://") + _homebus_server + "/provision");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("Authorization", _homebus_auth_token.c_str());

  Serial.print("token ");
  Serial.println(_homebus_auth_token);

  int httpCode = http.POST(buf);
  if(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
    String payload = http.getString();
    Serial.println("OKAY!");
    Serial.println(payload);

    homebus_process_response(payload);
  } else {
    Serial.print("httpCode is ");
    Serial.println(httpCode);
  }

  http.end();
}

/* 
 * response looks like:

{
          "status": "provisioned",
          "credentials": {
            "mqtt_username": "UUUUUUUU-WWWW-XXXX-YYYY-ZZZZZZZZZZZZ",
            "mqtt_password": "UUUUUUUU-WWWW-XXXX-YYYY-ZZZZZZZZZZZZ"
          },
          "broker": {
            "mqtt_hostname": "mqtt0.homebus.io",
            "insecure_mqtt_port": 1883,
            "secure_mqtt_port": 8883
          },
          "uuids": [ "UUUUUUUU-WWWW-XXXX-YYYY-ZZZZZZZZZZZZ" ],
          "refresh_token": "111111111122222222223333333333444444444455555555556666666666777777777788888888889999999999"
}

*/
void homebus_process_response(String payload) {
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + 750;
  StaticJsonDocument<capacity> doc;

  Serial.print("homebus response ");
  Serial.println(payload);

  Serial.print(payload.length());
  Serial.println(" bytes long");

  deserializeJson(doc, payload);

  if(strcmp(doc["status"], "waiting") == 0) {
    Serial.println("HOMEBUS_STATE_PROVISION_WAIT");

    refresh_token = String((const char *)doc["refresh_token"]);
    Serial.print("refresh token ");
    Serial.println(refresh_token);
    delay(500);

    homebus_state = HOMEBUS_STATE_PROVISION_WAIT;
    homebus_provision_retry_time = doc["retry_time"];
    homebus_next_provision_retry = millis() + homebus_provision_retry_time*1000;

    Serial.println("about to persist");
    delay(500);
  }

  if(strcmp(doc["status"], "provisioned") == 0) {
    Serial.println("HOMEBUS_STATE_OKAY");

    refresh_token = String((const char *)doc["refresh_token"]);

    mqtt_broker = String((const char *)doc["broker"]["mqtt_hostname"]);
    mqtt_port = doc["broker"]["insecure_mqtt_port"];

    mqtt_username = String((const char *)doc["credentials"]["mqtt_username"]);
    mqtt_password = String((const char *)doc["credentials"]["mqtt_password"]);

    UUID = String((const char *)doc["uuids"][0]);

    Serial.printf("Homebus broker name %s, insecure port %u\n", mqtt_broker.c_str(), mqtt_port);
    Serial.printf("Homebus credentials username %s, UUID %s\n", mqtt_username.c_str(), UUID.c_str());

    homebus_state = HOMEBUS_STATE_OKAY;
    homebus_mqtt_setup();
  }
    
  homebus_persist();
}

void homebus_publish(const char *msg) {
  if(homebus_state == HOMEBUS_STATE_OKAY)
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

  if(homebus_state == HOMEBUS_STATE_OKAY) {
    
    snprintf(topic, topic_len, "homebus/device/%s/%s", uuid, ddc);
    if(use_envelope) {
      snprintf(buf, buf_len, HOMEBUS_ENVELOPE, UUID.c_str(), time(NULL), ddc, msg);
    } else {
      
    }

    mqtt_publish(topic, buf, true);
  }
}

void homebus_publish_to(const char *ddc, const char *msg) {
  homebus_send_to(UUID.c_str(), ddc, msg);
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

  const char* source = doc["source"]; // "548de014-0389-438c-9a60-0f92fe37b4d0"
  unsigned long timestamp = doc["timestamp"];
  const char* ddc = doc["contents"]["ddc"];

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
