#pragma once

#include <Arduino.h>

#include <SPIFFS.h>
#include <FS.h>

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
