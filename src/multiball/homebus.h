#pragma once

#include <ArduinoJson.h>

typedef enum {
  HOMEBUS_STATE_NOT_SETUP = 0,           // not yet initialized
  HOMEBUS_STATE_FINDING_PROVISIONER = 1, // attempting to locate provisioner
  HOMEBUS_STATE_PROVISIONING = 2,        // attempting to contact provisioner
  HOMEBUS_STATE_PROVISION_WAIT = 3,      // waiting for provisioning response
  HOMEBUS_STATE_UPDATE_PASSWORD = 4,     // getting a new password from provisioner
  HOMEBUS_STATE_REJECTED = 5,            // provisioner rejected request
  HOMEBUS_STATE_OKAY = 6                 // up and running
} homebus_state_t;

void homebus_setup();
void homebus_handle();

void homebus_persist();
void homebus_restore();

void homebus_receive(const char* topic, char *msg, size_t length);

void homebus_system(JsonObject system);

void homebus_publish(const char* msg);
void homebus_publish_to(const char* topic, const char* msg);

String homebus_uuid();

void homebus_callback(const char* topic, const char* msg);

// friendly name, friendly location, manufacturer, model
void homebus_configure(const char*, const char*, const char*, const char*);

const char* homebus_mqtt_host();
const char* homebus_mqtt_username();
const char* homebus_mqtt_uuid();
uint16_t homebus_mqtt_port();
