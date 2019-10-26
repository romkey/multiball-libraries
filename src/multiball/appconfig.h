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
  AppConfig(const char* path = "/config/") : _path(path) {};

  boolean set(const char* key, const char* subkey, String value);
  String get(const char* key, const char* subkey, boolean *success = NULL);
  void clear(const char* key, const char* subkey);

  void migrate();

private:
  String _config_filename(const char* key, const char* subkey);
  static String _read_line_from_file(File fs);

  String _path;
};
