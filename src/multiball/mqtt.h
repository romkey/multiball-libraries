#pragma once

#include <Arduino.h>

#include "config.h"

#include <PubSubClient.h>

void mqtt_setup(const char* hostname, uint16_t port, const char* uuid, const char* username, const char* password);
void mqtt_handle();
void mqtt_subscribe(const char* topic);

//void mqtt_callback(const char*, const byte*, unsigned);

void mqtt_publish(const char* topic, const char* payload, bool retain = false);
bool mqtt_is_connected();

void mqtt_persist();
boolean mqtt_restore();
