#include <multiball/appconfig.h>

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

String AppConfig::get(const char* key, const char* subkey, boolean *success) {
  String path = _config_filename(key, subkey);

  File file = SPIFFS.open(path);
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

