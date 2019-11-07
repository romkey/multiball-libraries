#pragma once

#include <Arduino.h>

#include "config.h"

void mqtt_setup(String hostname, uint16_t port, String username, String password);
void mqtt_handle();
void mqtt_subscribe(const char* topic);

//void mqtt_callback(const char*, const byte*, unsigned);

void mqtt_publish(const char* topic, const char* payload, bool retain = false);
bool mqtt_is_connected();
