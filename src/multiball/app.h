#pragma once

#include <Arduino.h>

#include <multiball/wifi.h>
#include <multiball/appconfig.h>

class MultiballApp {
public:
  MultiballApp();

  void begin(const char* appname = "multiball-app");
  void handle();

  unsigned long uptime() { return millis() - _boot_time; };

  void reboot();
  void factory_reset();

  void serial_speed(unsigned speed) { _serial_speed = speed; };
  unsigned serial_speed() { return _serial_speed; };

  void wifi_credentials(uint8_t count, const wifi_credential_t *credentials);

  void status_changed(boolean status) { _status_changed = status; };
  boolean status_changed() { return _status_changed; };

  String mac_address() { return _mac_address; };
  String ip_address() { return _ip_address; };

  String build_info() { return _build_info; };
  unsigned wifi_failures();
  unsigned boot_count();

  AppConfig config;

  void updates_available(boolean status) { _updates_available = status; };
  boolean updates_available() { return _updates_available; };

private:
  unsigned _serial_speed = 115200;

  boolean _updates_available = false;

  String _mac_address;
  String _ip_address;
  String _hostname;
  String _build_info;

  volatile boolean _status_changed = false;

  wifi_credential_t *_wifi_credentials;
  uint8_t _number_of_wifi_credentials;

  unsigned long _boot_time;
};

extern class MultiballApp App;
