#pragma once

#include <Arduino.h>

#ifdef ESP32
#include <SPIFFS.h>
#include <FS.h>
#else
#include <FS.h>

#define FILE_READ "r"
#define FILE_WRITE "w"
#endif

class AppConfig {
public:
  void begin(const char* app_name);

  boolean set(const char* key, String value);
  String get(const char* key, boolean *success = NULL);
  boolean exists(const char* key);
  void clear(const char* key);

#ifdef ESP32
  String getv1(const char* key, boolean *success = NULL);
  void clearv1(const char* key);
#endif

private:
#ifdef ESP32
  boolean _initialized = false;
  uint32_t _handle;

  String _config_filename(const char* key);
  static String _read_line_from_file(File fs);
  String _path = "/config/";
#endif

#ifdef ESP8266
  String _path = "/config/";

  String _config_filename(const char* key);
  static String _read_line_from_file(File fs);
#endif

};
