#include <Arduino.h>
#include <SPIFFS.h>

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
#endif

  Serial.printf("Build %s\n", _build_info);

  if(!SPIFFS.begin(true))
    Serial.println("An Error has occurred while mounting SPIFFS");
  else
    Serial.println("[spiffs]");

  if(wifi_begin(_wifi_credentials, 3, app_name)) {
    _ip_address = String(WiFi.localIP());
    Serial.println(WiFi.localIP());
    Serial.println("[wifi]");

    byte mac_address[6];
    char mac_address_str[3 * 6];
    snprintf(mac_address_str, 3*6, "%02x:%02x:%02x:%02x:%02x:%02x", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);

    _mac_address = String(mac_address_str);

    _hostname = wifi_hostname();

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
