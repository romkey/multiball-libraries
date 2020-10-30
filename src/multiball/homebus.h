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
void homebus_reset();

void homebus_receive(const char *topic, char *msg, size_t length);

void homebus_system(JsonObject system);

void homebus_publish(const char *msg);
void homebus_publish_to(const char *topic, const char *msg);
void homebus_send_to(const char *uuid, const char *ddc, const char *msg);

void homebus_callback(const char *topic, const char *msg);

void homebus_configure(const char *manufacturer, const char *model, const char *serial_number, const char *pin, const char *write_only_ddcs[], const char *read_only_ddcs[]);
void homebus_set_provisioner(const char *server, const char *auth_token);

const char *homebus_uuid();

const char *homebus_mqtt_host();
const char *homebus_mqtt_username();
uint16_t homebus_mqtt_port();

/*
 * temporary function for transition to new libraries
 */
void homebus_stuff(const char *broker, uint16_t port, const char *username, const char *password, const char *uuid);
void homebus_mqtt_setup();
void homebus_mqtt_override_prefix(const char *prefix);
