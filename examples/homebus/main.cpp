#include <Arduino.h>

#include <multiball/app.h>
#include <multiball/wifi.h>
#include <multiball/ota_updates.h>
#include <multiball/homebus.h>

MultiballApp App;

void setup() {
  const wifi_credential_t wifi_credentials[] = {
    { "ssid1", "password1" },
    { "ssid2", "password2" }
  };

  App.wifi_credentials(2, wifi_credentials);
  App.begin("example");
  Serial.println("[app]");

  Serial.println("[homebus]");
  homebus_configure("Furball", "CTRLH Electronics Lab", "Homebus", "v4");
  homebus_setup();

}

void loop() {
  App.handle();
}
