#pragma once

extern homebus_state_t homebus_state;

extern String homebus_endpoint;
extern String homebus_cmd_endpoint;

extern String initial_auth_token;
extern String pr_token;
extern String pr_id;
extern String device_id;

extern String mqtt_username, mqtt_password, mqtt_broker;
extern uint16_t mqtt_port;

extern String homebus_provisioning_server;

extern boolean override_prefix;
extern boolean use_envelope;

extern const char *_manufacturer;
extern const char *_model;
extern const char *_serial_number;
extern const char *_pin;
extern const char **_publishes,** _consumes;

extern String _homebus_server, _homebus_auth_token;
