#include <Arduino.h>

#include <multiball/app.h>
#include <multiball/wifi.h>
#include <multiball/ota_updates.h>
#include <multiball/homebus.h>

MultiballApp App;

void setup() {
  const wifi_credential_t wifi_credentials[] = {
    { "ssid0", "password0" },
    { "ssid1", "password1" },
    { "ssid2", "password2" }
  };

  delay(500);

  App.wifi_credentials(3, wifi_credentials);
  App.begin("example");
  Serial.println("[app]");

  Serial.println("[homebus]");

  delay(500);

  homebus_set_provisioner("homebus.org", "HOMEBUS-API-KEY");

  const char *publishes[] = { "org.homebus.experimental.thermostat", NULL };
  const char *consumes[] = { "org.homebus.experimental.air-sensor", NULL };
  homebus_configure("Furball", "CTRLH Electronics Lab", "Homebus", "v4", publishes, consumes);

  homebus_setup();
}

void loop() {
  App.handle();
}
