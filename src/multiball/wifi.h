#pragma once

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#else
#include <Esp.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#endif

bool wifi_setup(char **wifi_credentials, unsigned count);
void wifi_handle();
const char* wifi_hostname();
