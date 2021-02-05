#include "config.h"

#include "multiball/wifi.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#else
#include <WiFi.h>
#include <WiFiMulti.h>
#endif

#ifdef ESP8266
static ESP8266WiFiMulti wifiMulti;
#else
static WiFiMulti wifiMulti;
#endif

#ifdef ESP8266
int wifi_failures = 0;
#else
RTC_DATA_ATTR int wifi_failures = 0;
#endif

bool wifi_begin(const  wifi_credential_t* wifi_credentials, unsigned count, const char* hostname) {
#ifdef ESP8266
  WiFi.hostname(hostname);
#else
  WiFi.setHostname(hostname);
#endif

  WiFi.persistent(false);

  for(unsigned i = 0; i < count; i++)
    wifiMulti.addAP(wifi_credentials[i].ssid, wifi_credentials[i].password);

  static int wifi_tries = 0;
  while(wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);

    if(wifi_tries++ > 100) {
      wifi_failures++;
      ESP.restart();
    }
  }

  return true;
}

void wifi_handle() {
  if(WiFi.status() != WL_CONNECTED)
    wifiMulti.run();
}
