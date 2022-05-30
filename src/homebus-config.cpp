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

// TECH DEBT
// this is awful, and temporary
// it was expedient to do this in breaking up the homebus code into multiple files
// this needs to be cleaned up and properly abstracted
static homebus_state_t _homebus_state = HOMEBUS_STATE_NOT_SETUP;

String homebus_endpoint;
String homebus_cmd_endpoint;

String device_id;
String device_token;
String initial_auth_token;
String pr_token;
String pr_id;
String mqtt_username, mqtt_password, mqtt_broker;
uint16_t mqtt_port = 0;

String homebus_provisioning_server;

boolean override_prefix = false;
boolean use_envelope = true;

const char *_manufacturer = "";
const char *_model = "";
const char *_serial_number = "";
const char *_pin = "";
const char **_publishes = { NULL }, **_consumes = { NULL };

String _homebus_server, _homebus_auth_token;

homebus_state_t homebus_state() {
  return _homebus_state;
}

void homebus_state(homebus_state_t state) {
  _homebus_state = state;
}

void homebus_persist() {
  Serial.println("homebus_persist()");

  App.config.set("hb-state", String(_homebus_state));
  App.config.set("hb-auth-token", initial_auth_token.c_str());
  App.config.set("hb-pr-id", pr_id.c_str());
  App.config.set("hb-pr-token", pr_token.c_str());
  App.config.set("hb-device-id", device_id.c_str());
  App.config.set("hb-broker", mqtt_broker.c_str());
  App.config.set("hb-username", mqtt_username.c_str());
  App.config.set("hb-password", mqtt_password.c_str());
  App.config.set("hb-port", String(mqtt_port).c_str());
}

void homebus_reset() {
  Serial.println("homebus_reset()");

  App.config.clear("hb-state");
  App.config.clear("hb-initial-auth-token");
  App.config.clear("hb-pr-id");
  App.config.clear("hb-pr-token");
  App.config.clear("hb-device-id");
  App.config.clear("hb-device-token");
  App.config.clear("hb-broker");
  App.config.clear("hb-username");
  App.config.clear("hb-password");
  App.config.clear("hb-port");
}


void homebus_restore() {
  boolean success;
  String temp;

  Serial.println("homebus_restore()");

#define HB_RESTORE_GET(KEY, VAR)   temp = App.config.get(KEY, &success); if(success) { VAR = temp; } else { Serial.println("homebus_restore: KEY missing"); }

  temp = App.config.get("hb-state", &success);
  if(success)
    _homebus_state = static_cast<homebus_state_t>(temp.toInt());

  temp = App.config.get("hb-auth-token", &success);
  if(success)
    initial_auth_token = temp;
  else
    Serial.println("Initial auth token fail");

  HB_RESTORE_GET("hb-pr-id", pr_id)
  HB_RESTORE_GET("hb-device-id", device_id)
  HB_RESTORE_GET("hb-device-token", device_token)
  HB_RESTORE_GET("hb-pr-token", pr_token)

  /*
  temp = App.config.get("hb-pr-id", &success);
  if(success)
    pr_id = temp;
  else
    Serial.println("PR ID fail");

  temp = App.config.get("hb-device-id", &success);
  if(success)
   device_id = temp;
  else
    Serial.println("Device ID fail");

  temp = App.config.get("hb-device-token", &success);
  if(success)
   device_token = temp;
  else
    Serial.println("Device ID fail");

  temp = App.config.get("hb-pr-token", &success);
  if(success)
    pr_token = temp;
  else
    Serial.println("PR token fail");
*/

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

  Serial.printf("homebus restore state %d broker %s port %u user %s pass %s pr_uuid %s pr_token %s device_id %s\n",
		static_cast<int>(_homebus_state),
		mqtt_broker.c_str(),
		mqtt_port,
		mqtt_username.c_str(),
		mqtt_password.c_str(),
		pr_id.c_str(),
		pr_token.c_str(),
		device_id.c_str());
}


// friendly name, friendly location, manufacturer, model, update_frequency
void homebus_configure(const char *manufacturer, const char *model, const char *serial_number, const char *pin, const char *publishes[], const char *consumes[]) {
  _manufacturer = manufacturer;
  _model = model;
  _serial_number = serial_number;
  _pin = pin;

  _publishes = publishes;
  _consumes = consumes;
}

void homebus_set_provisioner(const char *server, const char *auth_token) {
  _homebus_server = String(server);
  initial_auth_token = String(auth_token);
}

const char *homebus_uuid() {
  return device_id.c_str();
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
