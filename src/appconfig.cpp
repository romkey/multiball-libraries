#ifdef ESP32

#include <nvs.h>

#endif

#ifdef ESP8266
#include <LittleFS.h>
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
void AppConfig::begin(const char* app_name) {
  //  LittleFS.format();
  LittleFS.begin();
}

boolean AppConfig::set(const char* key, String value) {
  Serial.printf("AppConfig::set(%s, %s)\n", key, value.c_str());

  File f = LittleFS.open(_config_filename(key), FILE_WRITE);
  if(f) {
    f.print(value);
    f.close();
    return true;
  }

  return false;
}

String AppConfig::_config_filename(const char* key) {
  Serial.print("AppConfig::_config_filename ");
  Serial.println(_path + key);

  return _path + key;
}

String AppConfig::_read_line_from_file(File file, char *buffer, size_t buflen) {
  int i = 0;
  memset(buffer, 0, buflen);

  while(file.available()) {
    buffer[i++] = file.read();
    if(i == buflen - 1)
      break;
  }

  return String(buffer);
}

String AppConfig::get(const char* key, boolean *success) {
  String path = _config_filename(key);

  File file = LittleFS.open(path.c_str(), FILE_READ);
  Serial.printf("AppConfig::get file is %u bytes long\n", file.size());
  file.seek(0, SeekSet);

  if(!file || file.isDirectory()){
    Serial.println("AppConfig::get -> null");

    *success = false;
    return String("");
  }

  char buf[50];

  *success = true;
  String s = _read_line_from_file(file, buf, 50);

  Serial.printf("AppConfig::get(%s) -> %s\n", key, s.c_str());
  return s;
}

void AppConfig::clear(const char* key) {
  LittleFS.remove(_config_filename(key));
}
#endif
