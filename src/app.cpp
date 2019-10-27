#include <Arduino.h>

#ifdef ESP32
#include <SPIFFS.h>

#include "esp_system.h"
#else
#include <FS.h>

extern "C" {
#include <user_interface.h>
};

#endif

#include <multiball/app.h>

#include <multiball/wifi.h>
#include <multiball/ota_updates.h>
#include <multiball/mqtt.h>

// CPP weirdness to turn a bare token into a string
#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

// used to store persistent data across crashes/reboots
// cleared when power cycled or re-flashed
#ifdef ESP8266
int bootCount = 0;
#else
RTC_DATA_ATTR int bootCount = 0;
#endif

MultiballApp::MultiballApp() {
  bootCount++;
}

void MultiballApp::wifi_credentials(uint8_t count, const wifi_credential_t *credentials) {
  _number_of_wifi_credentials = count;
  _wifi_credentials = (wifi_credential_t *)malloc(sizeof(wifi_credential_t)*count);
  memcpy(_wifi_credentials, credentials, sizeof(wifi_credential_t)*count);
}

void MultiballApp::begin(const char* app_name) {
  Serial.begin(115200);
  Serial.println("Hello World!");

#ifdef BUILD_INFO
  _build_info = String(STRINGIZE(BUILD_INFO));
  Serial.printf("Build %s\n", _build_info.c_str());
#endif

  config.begin(app_name);

  if(!SPIFFS.begin())
    Serial.println("An Error has occurred while mounting SPIFFS");
  else
    Serial.println("[spiffs]");

  uint8_t hostname_len = strlen(app_name) + 1 + 6 + 1;
  char hostname[hostname_len];
  byte mac_address[6];
  char mac_address_str[3 * 6];

#ifdef ESP32
  esp_read_mac(mac_address, ESP_MAC_WIFI_STA);
#endif

#ifdef ESP8266
  wifi_get_macaddr(STATION_IF, mac_address);
#endif

  snprintf(mac_address_str, 3*6, "%02x:%02x:%02x:%02x:%02x:%02x", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
  _mac_address = String(mac_address_str);

  snprintf(hostname, hostname_len, "%s-%02x%02x%02x", app_name, mac_address[0], mac_address[1], mac_address[2]);
  _hostname = String(hostname);
  _default_hostname = true;

  restore();

  if(wifi_begin(_wifi_credentials, 3, _hostname.c_str())) {
    _ip_address = String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);
    Serial.println(WiFi.localIP());
    Serial.println("[wifi]");

    if(!MDNS.begin(_hostname.c_str()))
      Serial.println("Error setting up MDNS responder!");
    else
      Serial.println("[mDNS]");
  } else {
    Serial.println("wifi failure");
  }

  ota_updates_setup();
  Serial.println("[ota_updates]");

#ifdef USE_MQTT
  mqtt_setup(MQTT_HOST, MQTT_PORT, MQTT_UUID, MQTT_USER, MQTT_PASS);
  Serial.println("[mqtt]");
#endif
}

void MultiballApp::handle() {
  wifi_handle();
  ota_updates_handle();

#ifdef USE_MQTT
  mqtt_handle();
  homebus_mqtt_handle();
#endif
}

unsigned MultiballApp::boot_count() {
  return bootCount;
}

extern unsigned wifi_failures;

unsigned MultiballApp::wifi_failures() {
  return 0;
}

void MultiballApp::persist() {
  if(!_default_hostname)
    config.set("app-hostname", _hostname);
}

void MultiballApp::restore() {
  boolean success = false;
  String configured_hostname;

  configured_hostname = config.get("app-hostname", &success);
  if(success) {
    _default_hostname = false;
    _hostname = configured_hostname;
  }
}
