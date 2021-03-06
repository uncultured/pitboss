#include <PitBoss/Config.h>
#include <WiFiManager.h>
#include <map>
#include <vector>

namespace PitBoss {

std::vector<String> Config::fromJson(StaticJsonDocument<Config::CONFIG_FILE_MAX_SIZE> json) {
  std::vector<String> errors;
  if (json.containsKey(Config::jsonKeys::LOG_LEVEL)) {
    this->logLevel = json[Config::jsonKeys::LOG_LEVEL].as<int>();
  }
  if (json.containsKey(Config::jsonKeys::WIFI_COUNTRY)) {
    std::map<const char*, wifi_country_t> wifiCountries;
    wifiCountries[WM_COUNTRY_US.cc] = WM_COUNTRY_US;
    wifiCountries[WM_COUNTRY_CN.cc] = WM_COUNTRY_CN;
    wifiCountries[WM_COUNTRY_JP.cc] = WM_COUNTRY_JP;
    const char* countryCode = json[Config::jsonKeys::WIFI_COUNTRY];
    wifi_country_t* country = Config::getCountryFromCode(countryCode);
    if (!country) {
      auto temp = String(F("Unknown country: %s"));
      char buffer[temp.length() + sizeof(countryCode)];
      sprintf(buffer, temp.c_str(), countryCode);
      errors.push_back(String(buffer));
    }
  }
  if (json.containsKey(Config::jsonKeys::NTP_SERVER)) {
    this->ntpServer = json[Config::jsonKeys::NTP_SERVER].as<char *>();
  }
  if (json.containsKey(Config::jsonKeys::THERMOCOUPLE_READ_INTERVAL_MS)) {
    this->thermocoupleReadInterval = json[Config::jsonKeys::THERMOCOUPLE_READ_INTERVAL_MS].as<int>();
  }
  if (json.containsKey(Config::jsonKeys::GMT_OFFSET)) {
    this->gmtOffset = json[Config::jsonKeys::GMT_OFFSET].as<int>();
  }
  if (json.containsKey(Config::jsonKeys::DST_OFFSET)) {
    this->dstOffset = json[Config::jsonKeys::DST_OFFSET].as<int>();
  }
  return errors;
}

wifi_country_t *Config::getCountryFromCode(const String &code) {
  wifi_country_t* country = nullptr;
  std::map<String, wifi_country_t> wifiCountries;
  wifiCountries[WM_COUNTRY_US.cc] = WM_COUNTRY_US;
  wifiCountries[WM_COUNTRY_CN.cc] = WM_COUNTRY_CN;
  wifiCountries[WM_COUNTRY_JP.cc] = WM_COUNTRY_JP;
  auto pos = wifiCountries.find(code);
  if (pos != wifiCountries.end()) {
    country = &(wifiCountries[code]);
  }
  return country;
}

StaticJsonDocument<Config::CONFIG_FILE_MAX_SIZE> Config::toJson() {
  StaticJsonDocument<Config::CONFIG_FILE_MAX_SIZE> json;
  json[Config::jsonKeys::LOG_LEVEL] = this->logLevel;
  json[Config::jsonKeys::WIFI_COUNTRY] = this->wifiCountry.cc;
  json[Config::jsonKeys::NTP_SERVER] = this->ntpServer;
  json[Config::jsonKeys::GMT_OFFSET] = this->gmtOffset;
  json[Config::jsonKeys::DST_OFFSET] = this->dstOffset;
  json[Config::jsonKeys::THERMOCOUPLE_READ_INTERVAL_MS] = this->thermocoupleReadInterval;
  return json;
}

}