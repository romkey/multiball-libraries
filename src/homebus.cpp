#include <multiball/app.h>
#include <multiball/homebus.h>
#include <multiball/mqtt.h>

#include <ESPmDNS.h>
#include <HTTPClient.h>

static String UUID;
static String mqtt_username, mqtt_password, mqtt_broker;
static uint16_t mqtt_port = 0;

const char* homebus_mqtt_uuid() {
  return UUID.c_str();
}

const char* homebus_mqtt_username() {
  return mqtt_username.c_str();
}

const char* homebus_mqtt_host() {
  return mqtt_broker.c_str();
}

uint16_t homebus_mqtt_port() {
  return mqtt_port;
}

static String homebus_endpoint;
static String homebus_cmd_endpoint;
static boolean provisioned = false;

static homebus_state_t homebus_state = HOMEBUS_STATE_NOT_SETUP;

static unsigned long homebus_next_provision_retry = 0;
static unsigned homebus_provision_retry_time = 60;

static void homebus_provision();
static void homebus_provision_request(char*buf, size_t);
static void homebus_process_response(String payload);

static const char* _friendly_name = "", *_friendly_location = "", *_manufacturer = "", *_model = "";

// friendly name, friendly location, manufacturer, model, update_frequency
void homebus_configure(const char* friendly_name, const char* friendly_location, const char* manufacturer, const char* model) {
  _friendly_name = friendly_name;
  _friendly_location = friendly_location;
  _manufacturer = manufacturer;
  _model = model;
}

void homebus_uuid(String new_uuid) {
  UUID = new_uuid;
  homebus_endpoint = "/homebus/device/" + UUID;
  homebus_cmd_endpoint = "/homebus/device/" + UUID + "/cmd";
}

String homebus_uuid() {
  return UUID;
}

void homebus_mqtt_setup() {
  mqtt_setup(mqtt_broker, mqtt_port, mqtt_username, mqtt_password);
  mqtt_subscribe((homebus_endpoint + "/#").c_str());
}

void homebus_setup() {
  homebus_restore();

  if(homebus_state == HOMEBUS_STATE_OKAY) {
    return;
  }

  IPAddress homebus_ip = MDNS.queryHost("homebus.local");
  Serial.print("homebus IP is ");
  Serial.println(homebus_ip);

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

// labels must be 15 characters or fewer, so use "hb-" as a prefix, not "homebus-"
void homebus_persist() {
  App.config.set("hb-state", String(homebus_state));
  App.config.set("hb-uuid", UUID.c_str());
  App.config.set("hb-broker", mqtt_broker.c_str());
  App.config.set("hb-username", mqtt_username.c_str());
  App.config.set("hb-password", mqtt_password.c_str());
  App.config.set("hb-port", String(mqtt_port).c_str());
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
 * {
 *     "friendly_name": "LED strip",
 *     "friendly_location": "Hipster Hideaway",
 *     "manufacturer": "HomeBus",
 *     "model": "v0",
 *     "serial_number": "7",
 *     "pin": "",
 *     "devices": [
 *       { "friendly_name": "LED strip",
 *         "friendly_location": "",
 *         "update_frequency": 0,
 *         "index": 0,
 *         "accuracy": 0,
 *         "precision": 0,
 *         "wo_topics": [ ],
 *         "ro_topics": [ "preset", "animation", "animation_speed", "brightness", "maximum_brightness" ],
 *         "rw_topics": []
 *       },
 *       { "friendly_name": "temperature",
 *         "friendly_location": "",
 *         "update_frequency": 60,
 *         "index": 0,
 *         "accuracy": 0,
 *         "precision": 0,
 *         "wo_topics": [ "temperature", "humidity", "pressure" ],
 *         "ro_topics": [ ],
 *         "rw_topics": []
 *       }
 *    ]
 *  }
 */

static void homebus_provision_request(char *buf, size_t buf_length) {
  const size_t capacity = 4*JSON_ARRAY_SIZE(0) + JSON_ARRAY_SIZE(2) + JSON_ARRAY_SIZE(3) + JSON_ARRAY_SIZE(5) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(7) + 2*JSON_OBJECT_SIZE(9);
  StaticJsonDocument<capacity> doc;

  JsonObject provision = doc.createNestedObject("provision");
  provision["friendly_name"] = _friendly_name;
  provision["friendly_location"] = _friendly_location;
  provision["manufacturer"] = _manufacturer;
  provision["model"] = _model;
  provision["serial_number"] = App.mac_address();
  provision["pin"] = "";

  JsonArray provision_devices = provision.createNestedArray("devices");

  JsonObject provision_devices_0 = provision_devices.createNestedObject();
  provision_devices_0["friendly_name"] = "LED strip";
  provision_devices_0["friendly_location"] = "";
  provision_devices_0["update_frequency"] = 0;
  provision_devices_0["index"] = 0;
  provision_devices_0["accuracy"] = 0;
  provision_devices_0["precision"] = 0;
  JsonArray provision_devices_0_wo_topics = provision_devices_0.createNestedArray("wo_topics");

  JsonArray provision_devices_0_ro_topics = provision_devices_0.createNestedArray("ro_topics");
  provision_devices_0_ro_topics.add("preset");
  provision_devices_0_ro_topics.add("animation");
  provision_devices_0_ro_topics.add("animation_speed");
  provision_devices_0_ro_topics.add("brightness");
  provision_devices_0_ro_topics.add("maximum_brightness");
  JsonArray provision_devices_0_rw_topics = provision_devices_0.createNestedArray("rw_topics");

  JsonObject provision_devices_1 = provision_devices.createNestedObject();
  provision_devices_1["friendly_name"] = "temperature";
  provision_devices_1["friendly_location"] = "";
  provision_devices_1["update_frequency"] = 60;
  provision_devices_1["index"] = 0;
  provision_devices_1["accuracy"] = 0;
  provision_devices_1["precision"] = 0;

  JsonArray provision_devices_1_wo_topics = provision_devices_1.createNestedArray("wo_topics");
  provision_devices_1_wo_topics.add("temperature");
  provision_devices_1_wo_topics.add("humidity");
  provision_devices_1_wo_topics.add("pressure");
  JsonArray provision_devices_1_ro_topics = provision_devices_1.createNestedArray("ro_topics");
  JsonArray provision_devices_1_rw_topics = provision_devices_1.createNestedArray("rw_topics");
  
  serializeJsonPretty(doc, buf, buf_length);
  Serial.println(strlen(buf));
  Serial.println(buf);

  serializeJson(doc, buf, buf_length);
  Serial.println(strlen(buf));
  Serial.println(buf);
}

void homebus_receive(const char* topic, char *msg, size_t length) {
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

static void homebus_provision() {
  char buf[1024];
  HTTPClient http;

  homebus_provision_request(buf, 1024);

  //  http.begin("http://hipster.homebus.io/provision");
  http.begin("http://ctrlh.homebus.io:5678/provision");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  
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

void homebus_process_response(String payload) {
  const size_t capacity = JSON_ARRAY_SIZE(2) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(7) + 550;
  StaticJsonDocument<capacity> doc;

  Serial.print("homebus response ");
  Serial.println(payload);

  deserializeJson(doc, payload);

  const char* UUID_c_str = doc["uuid"];
  UUID = UUID_c_str;

  if(strcmp(doc["status"], "waiting") == 0) {
    Serial.println("HOMEBUS_STATE_PROVISION_WAIT");

    homebus_state = HOMEBUS_STATE_PROVISION_WAIT;
    homebus_provision_retry_time = atoi(doc["retry_time"]);
    homebus_next_provision_retry = millis() + homebus_provision_retry_time*1000;
  }

  if(strcmp(doc["status"], "provisioned") == 0) {
    const char* str;

    Serial.println("HOMEBUS_STATE_OKAY");

    str = doc["mqtt_hostname"];
    mqtt_broker = str;

    Serial.println("RESP hostname ");
    Serial.println(mqtt_broker);

    str = doc["mqtt_username"];
    mqtt_username = str;

    Serial.print("RESP username ");
    Serial.println(mqtt_username);

    str = doc["mqtt_password"];
    mqtt_password = str;
    mqtt_port = doc["mqtt_port"];

    homebus_state = HOMEBUS_STATE_OKAY;
    homebus_mqtt_setup();
  }
    
  homebus_persist();
}

void homebus_publish(const char *msg) {
  mqtt_publish(homebus_endpoint.c_str(), msg, true);
}

void homebus_publish_to(const char *topic, const char *msg) {
  mqtt_publish(("/homebus/device/" + UUID + "/" + topic).c_str(), msg, true);
}

void homebus_mqtt_callback(const char* topic, const char* msg) {

}
