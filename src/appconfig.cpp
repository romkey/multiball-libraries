#include <multiball/appconfig.h>

#include <SPIFFS.h>
#include <FS.h>

boolean AppConfig::set(const char* key, const char* subkey, String value) {
  File f = SPIFFS.open(_config_filename(key, subkey), FILE_WRITE);
  if(f) {
    f.println(value);
    f.close();
    return true;
  }

  return false;
}

String AppConfig::_config_filename(const char* key, const char* subkey) {
  if(subkey == "")
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

String AppConfig::get(const char* key, const char* subkey, boolean *success) {
  File file = SPIFFS.open(_config_filename(key, subkey), FILE_READ);
  if(file) {
    *success = true;
    return String(_read_line_from_file(file));
  }
}

void AppConfig::clear(const char* key, const char* subkey) {
  SPIFFS.remove(_config_filename(key, subkey));
}

