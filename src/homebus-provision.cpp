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

static unsigned long homebus_next_provision_retry = 0;
static unsigned homebus_provision_retry_interval = 30; // seconds

static void homebus_provision();
static void homebus_provision_start();
static void homebus_provision_wait();

static void homebus_provision_request(char*buf, size_t);
static void homebus_process_response(String payload, int code);

void homebus_setup() {
  homebus_restore();

#if 0
  if(homebus_state == HOMEBUS_STATE_SUCCESS) {
    homebus_mqtt_setup();
    mqtt_connect();
    return;
  }
#endif

  homebus_provision();
}

void homebus_handle() {
  if(homebus_state == HOMEBUS_STATE_PROVISION_WAIT) {
    if(homebus_next_provision_retry < millis()) {
      Serial.println("homebus provision retrying...");
      homebus_next_provision_retry = millis() + homebus_provision_retry_interval*1000;
      homebus_provision();
    }
  }

  if(homebus_state == HOMEBUS_STATE_SUCCESS && !mqtt_is_connected()) {
    homebus_mqtt_setup();
    mqtt_connect();
  }
}

static void homebus_provision() {
  switch(homebus_state) {
  case HOMEBUS_STATE_NOT_SETUP:
    if(_homebus_server != "" && initial_auth_token != "")
      homebus_state = HOMEBUS_STATE_PROVISION_START;
    else
      break;
  case HOMEBUS_STATE_PROVISION_START:
    homebus_provision_start();
    break;
  case HOMEBUS_STATE_PROVISION_WAIT:
    homebus_provision_wait();
    break;
  case HOMEBUS_STATE_FINDING_PROVISIONER:
  case HOMEBUS_STATE_UPDATE_CREDENTIALS:
  case HOMEBUS_STATE_REJECTED:
  case HOMEBUS_STATE_UNKNOWN:
  case HOMEBUS_STATE_SUCCESS:
  default:
    return;
  }
}



/*
 * generate C++ code at https://arduinojson.org/v6/assistant/
 *
 *
  {
  	"name": "Furball sensor",
	"publishes": ["org.homebus.experimental.thermostat"],
	"consumes": ["org.homebus.experimental.air-sensor"]
  	"devices": [{
  		"identity": {
  			"manufacturer": "HomeBus",
  			"model": "v0",
  			"serial_number": "7",
  			"pin": ""
  		}
  	}]
  }
 */

static void homebus_provision_request(char *buf, size_t buf_length) {
  StaticJsonDocument<384> doc;

  doc["name"] = "Furball sensor";

  JsonArray devices = doc.createNestedArray("devices");
  JsonObject devices_0_identity = devices[0].createNestedObject("identity");
  devices_0_identity["manufacturer"] = _manufacturer;
  devices_0_identity["model"] = _model;
  devices_0_identity["serial_number"] = _serial_number;
  devices_0_identity["pin"] = _pin;

  JsonArray ddcs_publishes = doc.createNestedArray("publishes");

  const char **ptr = _publishes;
  while(1) {
    if(*ptr == NULL)
      break;

    Serial.printf("adding %s to publishes\n", *ptr);
    ddcs_publishes.add(*ptr);
    ptr++;
  }

  ptr = _consumes;
  JsonArray ddcs_consumes = doc.createNestedArray("consumes");

  while(1) {
    if(*ptr == NULL)
      break;

    Serial.printf("adding %s to consumes\n", *ptr);
    ddcs_consumes.add(*ptr);
    ptr++;
  }

  serializeJson(doc, buf, buf_length);
  Serial.println(strlen(buf));
  Serial.println(buf);
}

static void homebus_provision_start() {
  HTTPClient http;
  String url = String("http://") + _homebus_server + "/api/provision_requests";
  char buf[1024];

  Serial.print("homebus_provision_start: provisioner URL is ");
  Serial.println(url);

  homebus_provision_request(buf, 1024);

  http.begin(url);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("Authorization", "Bearer " + initial_auth_token);

  Serial.print("token ");
  Serial.println(initial_auth_token);

  int httpCode = http.POST(buf);
  if(httpCode == HTTP_CODE_OK || httpCode == 202) {
    String payload = http.getString();
    Serial.println("OKAY!");
    Serial.println(payload);

    homebus_process_response(payload, httpCode);
  } else {
    Serial.print("httpCode is ");
    Serial.println(httpCode);
  }

  http.end();
}

static void homebus_provision_wait() {
  HTTPClient http;
  String url = String("http://") + _homebus_server + "/api/provision_requests/" + pr_id;

  Serial.print("homebus_provision_wait: provisioner URL is ");
  Serial.println(url);

  http.begin(url);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("Authorization", "Bearer " + pr_token);

  Serial.print("token ");
  Serial.println(pr_token);

  int httpCode = http.GET();
  if(httpCode == HTTP_CODE_OK || httpCode == 202) {
    String payload = http.getString();
    Serial.println("OKAY!");
    Serial.println(payload);

    homebus_process_response(payload, httpCode);
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
   "provision_request": {
     "id": "UUUUUUUU-WWWW-XXXX-YYYY-ZZZZZZZZZZZZ",
     "token": "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
     "publishes": ["org.homebus.experimental.thermostat"],
     "consumes": ["org.homebus.experimental.air-sensor"]
   },
   "broker": {
     "mqtt_hostname": "mqtt0.homebus.io",
     "insecure_mqtt_port": 1883,
     "secure_mqtt_port": 8883
     },
   "credentials": {
     "mqtt_username": "UUUUUUUU-WWWW-XXXX-YYYY-ZZZZZZZZZZZZ",
     "mqtt_password": "UUUUUUUU-WWWW-XXXX-YYYY-ZZZZZZZZZZZZ"
     },
   "devices": [
     "id": "UUUUUUUU-WWWW-XXXX-YYYY-ZZZZZZZZZZZZ",
     "token": "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY",
     "identity": {
       "manufacturer": "HomeBus",
       "model": "v0",
       "serial_number": "7",
       "pin": ""
     }
   ]
 }

*/
void homebus_process_response(String payload, int status) {
  StaticJsonDocument<2048> doc;

  Serial.println("HOMEBUS PROCESS RESPONSE");
  Serial.println(payload);

  DeserializationError error = deserializeJson(doc, payload.c_str(), payload.length());

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  boolean homebus_validate_provision_request_response(StaticJsonDocument<1024> doc);

  if(!homebus_validate_provision_request_response(doc)) {
    Serial.println("invalid provision_request response");
    return;
  }    

  if(doc.containsKey("retry_interval")) {
    Serial.println("HOMEBUS_STATE_PROVISION_WAIT");

    pr_id = String((const char*)doc["provision_request"]["id"]);
    Serial.print("pr id ");
    Serial.println(pr_id);
    delay(500);

    pr_token = String((const char *)doc["provision_request"]["token"]);
    Serial.print("pr token ");
    Serial.println(pr_token);
    delay(500);

    homebus_state = HOMEBUS_STATE_PROVISION_WAIT;
    homebus_provision_retry_interval = doc["retry_interval"];
    homebus_next_provision_retry = millis() + homebus_provision_retry_interval*1000;

    Serial.println("about to persist");
    homebus_persist();
    return;
  }

  Serial.print("homebus response ");
  Serial.println(payload);

  Serial.print(payload.length());
  Serial.println(" bytes long");

  Serial.println("HOMEBUS_STATE_SUCCESS");

  pr_token = String((const char *)doc["provision_request"]["token"]);

  mqtt_broker = String((const char *)doc["broker"]["mqtt_hostname"]);
  mqtt_port = doc["broker"]["insecure_mqtt_port"];

  mqtt_username = String((const char *)doc["credentials"]["mqtt_username"]);
  mqtt_password = String((const char *)doc["credentials"]["mqtt_password"]);

  JsonObject device = doc["devices"][0];
  const char *id = device["id"];
  device_id = String(id);

  Serial.printf("Homebus broker name %s, insecure port %u\n", mqtt_broker.c_str(), mqtt_port);
  Serial.printf("Homebus credentials username %s, pr id %s, device id %s\n", mqtt_username.c_str(), pr_id.c_str(), device_id.c_str());

  homebus_state = HOMEBUS_STATE_SUCCESS;
  homebus_persist();
}

boolean homebus_validate_provision_request_response(StaticJsonDocument<2048> doc) {
  if(!doc.containsKey("provision_request")) {
    Serial.println("no provision_request!");
    return false;
  }

  if(!doc["provision_request"].containsKey("id")) {
    Serial.println("no provision_request id");
    return false;
  }

  if(!doc["provision_request"].containsKey("token")) {
    Serial.println("no provision_request token");
    return false;
  }

  if(doc.containsKey("retry_interval")) {
    Serial.println("found retry_interval");
    return true;
  }

  if(!doc.containsKey("credentials")) {
    Serial.println("credentials");
    return false;
  }

  if(!doc["credentials"].containsKey("mqtt_username")) {
    Serial.println("credentials username");
    return false;
  }

  if(!doc["credentials"].containsKey("mqtt_password")) {
    Serial.println("credentials password");
    return false;
  }

  if(!doc.containsKey("devices")) {
    Serial.println("devices");
    return false;
  }

  if(!doc["devices"][0]) {
    Serial.println("first device");
    return false;
  }

  if(!doc["devices"][0].containsKey("id")) {
    Serial.println("device ID");
    return false;
  }

  if(!doc.containsKey("broker")) {
    Serial.println("no broker");
    return false;
  }

  if(!doc["broker"].containsKey("mqtt_hostname")) {
    Serial.println("no broker hostname");
    return false;
  }

  if(!doc["broker"].containsKey("insecure_mqtt_port")) {
    Serial.println("no broker port");
    return false;
  }

  return true;
}
