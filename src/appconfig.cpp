#ifdef ESP32

#include <nvs.h>

#endif

#include <multiball/appconfig.h>

#ifdef ESP32
void AppConfig::begin(const char* app_name) {
  esp_err_t err = nvs_open(app_name, NVS_READWRITE, &_handle);
  if(!err)
    _initialized = true;
}

boolean AppConfig::set(const char* key, String value) {
  if(!_initialized) {
    return false;
  }

  esp_err_t err = nvs_set_str(_handle, key, value.c_str());
  if(err) {
    return false;
  }

  err = nvs_commit(_handle);
  if(err) {
    return false;
  }

  return true;
}

String AppConfig::get(const char* key, boolean *success) {
  if(!_initialized) {
    *success = false;
    return String("");
  }

  size_t len;
  esp_err_t err = nvs_get_str(_handle, key, NULL, &len);
  if(err) {
    *success = false;
    return String("");
  }

  char buf[len];
  err = nvs_get_str(_handle, key, buf, &len);
  if(err) {
    *success = false;
    return String("");
  }

  *success = true;
  return String(buf);
}

void AppConfig::clear(const char* key) {
  nvs_erase_key(_handle, key);
}

boolean AppConfig::exists(const char* key) {
  size_t len;
  esp_err_t err = nvs_get_str(_handle, key, NULL, &len);
  return err;
}
#endif

#ifdef ESP8266
boolean AppConfig::set(const char* key, String value) {
  File f = SPIFFS.open(_config_filename(key, subkey), FILE_WRITE);
  if(f) {
    f.println(value);
    f.close();
    return true;
  }

  return false;
}

String AppConfig::_config_filename(const char* key, const char* subkey) {
  if(strlen(subkey) == 0)
    return _path + key;
  else
    return _path + key + "_" + subkey;
}

String AppConfig::_read_line_from_file(File file) {
  static char buffer[32];
  while(file.available()) {
    int length = file.readBytesUntil('\n', buffer, sizeof(buffer));
    buffer[length > 0 ? length - 1 : 0] =  '\0';
  }

  return buffer;
}

String AppConfig::get(const char* key, boolean *success) {
  String path = _config_filename(key, subkey);

  File file = SPIFFS.open(path.c_str(), FILE_READ);
  if(!file || file.isDirectory()){
    *success = false;
    return String("");
  }

  *success = true;
  String s = _read_line_from_file(file);
  return s;
}

void AppConfig::clear(const char* key, const char* subkey) {
  SPIFFS.remove(_config_filename(key, subkey));
}
#endif
