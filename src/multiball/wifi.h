#pragma once

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#else
#include <Esp.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#endif

typedef struct {
  const char* ssid;
  const char* password;
} wifi_credential_t;

bool wifi_begin(const wifi_credential_t *credentials, unsigned count, const char* hostname_prefix = "multiball-host");
void wifi_handle();
const char* wifi_hostname();
