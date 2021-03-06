#pragma once

#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ArduinoLog.h>

namespace PitBoss {

struct Config {
  constexpr static const char* DEFAULT_NTP_SERVER = "pool.ntp.org";
  constexpr static const int DEFAULT_THERMOCOUPLE_READ_INTERVAL = 2000;
  constexpr static const int CONFIG_FILE_MAX_SIZE = 1024;
  struct jsonKeys {
    constexpr static const char* WIFI_COUNTRY = "wifiCountry";
    constexpr static const char* NTP_SERVER = "ntpServer";
    constexpr static const char* GMT_OFFSET = "gmtOffset";
    constexpr static const char* DST_OFFSET = "dstOffset";
    constexpr static const char* LOG_LEVEL = "logLevel";
    constexpr static const char* THERMOCOUPLE_READ_INTERVAL_MS = "thermocoupleReadInterval";
  };
  std::vector<String> fromJson(StaticJsonDocument<Config::CONFIG_FILE_MAX_SIZE> json);
  StaticJsonDocument<Config::CONFIG_FILE_MAX_SIZE> toJson();

  bool debug = true;
  wifi_country_t wifiCountry = WM_COUNTRY_US;
  String ntpServer = DEFAULT_NTP_SERVER;
  int gmtOffset = 0;
  int dstOffset = 0;
  int logLevel = LOG_LEVEL_VERBOSE;
  int thermocoupleReadInterval = DEFAULT_THERMOCOUPLE_READ_INTERVAL;
 protected:
  static wifi_country_t* getCountryFromCode(const String &code);
};

}