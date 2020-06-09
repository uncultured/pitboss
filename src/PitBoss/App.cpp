#include <SPIFFS.h>
#include <PitBoss/App.h>

namespace PitBoss {

void App::setup() {
  Serial.begin(App::BAUD_RATE);
  this->initStatusLED();
  this->initLog();
  this->setState(ApplicationStates::State::BOOTING);
  if (!this->initSPIFFS()) {
    this->_log->fatal(F("Unable to initialize SPIFFS."));
    this->setState(ApplicationStates::State::FATAL_ERROR);
    return;
  }
  App::splashScreen();
  if (!this->initConfig()) {
    this->setState(ApplicationStates::State::FATAL_ERROR);
    return;
  }
  this->initWebServer();
  this->initNtp();
  this->initWifi();
  this->initThermocouple();
  this->_resetButton.begin();
}

void App::initStatusLED() {
  this->onState(ApplicationStates::State::BOOTING, [this](){
    this->_statusLED.waiting();
  });
  this->onState(ApplicationStates::State::FATAL_ERROR, [this](){
    this->_statusLED.error();
  });
  this->onState(ApplicationStates::State::PROVISIONING, [this](){
    this->_statusLED.waiting();
  });
  this->onState(ApplicationStates::State::DISCONNECTED, [this](){
    this->_statusLED.waiting();
  });
  this->onState(ApplicationStates::State::READY, [this](){
    this->_statusLED.ready();
  });
  this->onState(ApplicationStates::State::THERMOCOUPLE_ERROR, [this](){
    this->_statusLED.error();
  });
  this->_statusLED.setup();
}

void App::initLog() {
  this->_log->setPrefix([](Print* _logOutput){
    _logOutput->print("pitboss: ");
  });
  this->_log->setSuffix([](Print* _logOutput){
    _logOutput->print('\n');
  });
  this->_log->begin(this->_config.logLevel, &Serial);
  this->_log->notice(F("Logging started"));
}

bool App::initSPIFFS() {
  return SPIFFS.begin(false);
}

bool App::initConfig() {
  File configFile = SPIFFS.open(App::DEFAULT_CONFIG_FILE_PATH);
  StaticJsonDocument<Config::CONFIG_FILE_MAX_SIZE> configJson;
  String configBuffer;
  if (configFile) {
    while (configFile.available()) {
      configBuffer += configFile.readString();
    }
    DeserializationError err = deserializeJson(configJson, configBuffer);
    if (err == DeserializationError::Ok) {
      auto errors = this->_config.fromJson(configJson);
      if (!errors.empty()) {
        for (const auto& it : errors) {
          const String& expectString(it);
          this->_log->fatal(F("Config error: %s"), expectString.c_str());
        }
        return false;
      }
    } else {
      this->_log->fatal(F("Unable to parse config file: %s"), err.c_str());
      return false;
    }
  } else {
    this->_log->warning(F("Unable to locate config file, using defaults"));
  }
  configBuffer.clear();
  serializeJsonPretty(this->_config.toJson(), configBuffer);
  this->_log->notice(F("Using config: \n%s"), configBuffer.c_str());
  return true;
}

void App::initWebServer() {
  this->_webServer.onNotFound([](AsyncWebServerRequest *request){
    Log.notice("404");
  });
  this->_webServer.on("/", [](AsyncWebServerRequest *request){
    request->redirect("/temperature");
  });
  this->_webServer.on("/temperature", HTTP_GET, [this](AsyncWebServerRequest *request){
    tm time;
    if (!getLocalTime(&time)) {
      request->send(500);
      return;
    }
    double coldJunction;
    double hotJunction;
    if (!this->_thermocouple.readThermocouple(coldJunction, hotJunction)) {
      request->send(500);
      return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    StaticJsonDocument<256> json;
    json["time"] = App::formatDateTime(&time);
    json["coldJunction"] = App::celsiusToFarenheit(coldJunction);
    json["hotJunction"] = App::celsiusToFarenheit(hotJunction);
    auto debug = json.createNestedObject("debug");
    debug["heap"] = ESP.getFreeHeap();
    debug["rssi"] = this->_wifi.getSignalStrength();
    debug["ssid"] = this->_wifi.getSSID();
    response->setCode(200);
    auto bytesWritten = serializeJson(json, *response);
    if (bytesWritten == 0) {
      this->_log->error(F("Serialization failure."));
      request->send(500);
      return;
    }
    request->send(response);
  });
  this->_webServer.on("/config", HTTP_GET, [this](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    auto config = this->_config.toJson();
    response->setCode(200);
    auto bytesWritten = serializeJson(config, *response);
    if (bytesWritten == 0) {
      this->_log->error(F("Serialization failure."));
      request->send(500);
      return;
    }
    request->send(response);
  });
  this->_webServer.on("/config", HTTP_POST, [this](AsyncWebServerRequest *request){
    request->send(501, "text/plain", "Not implemented");
  });
  this->onState(ApplicationStates::State::READY, [this](){
    this->_log->notice(F("Starting web server"));

    this->_webServer.begin();
  });
  this->onState(ApplicationStates::State::DISCONNECTED, [this](){
    this->_log->notice(F("Stopping web server"));
    this->_webServer.end();
  });
}

void App::initNtp() {
  this->onState(ApplicationStates::State::READY, [this](){
    if (!this->_config.ntpServer.equals(Config::DEFAULT_NTP_SERVER)) {
      configTime(0, 0, this->_config.ntpServer.c_str(), Config::DEFAULT_NTP_SERVER, WiFi.gatewayIP().toString().c_str());
    } else {
      configTime(0, 0, Config::DEFAULT_NTP_SERVER, WiFi.gatewayIP().toString().c_str());
    }
    tm time;
    getLocalTime(&time);
    this->_log->notice(F("Got time: %s"), App::formatDateTime(&time).c_str());
  });
}

void App::initWifi() {
  this->_wifi.onState(StatefulWiFiStates::State::CONNECTED, [this](){
    this->setState(ApplicationStates::State::READY);
  });
  this->_wifi.onState(StatefulWiFiStates::State::DISCONNECTED, [this](){
    this->setState(ApplicationStates::State::DISCONNECTED);
  });
  this->_wifi.onState(StatefulWiFiStates::State::ERROR, [this](){
    this->setState(ApplicationStates::State::FATAL_ERROR);
  });
  this->_wifi.setup();
}

void App::initThermocouple() {
  this->_thermocouple.onState(StatefulThermocoupleStates::State::READY, [this](){
    if (this->_thermocouple.getPreviousState() == StatefulThermocoupleStates::State::ERROR) {
      this->_log->notice(F("Thermocouple has been reconnected."));
    }
    this->setState(this->_wifi.getState() == StatefulWiFiStates::State::CONNECTED ? ApplicationStates::State::READY : ApplicationStates::State::DISCONNECTED);
  });
  this->_thermocouple.onState(StatefulThermocoupleStates::State::ERROR, [this](){
    this->_log->notice(F("Thermocouple has been disconnected. Will continue polling for reconnection."));
    this->setState(ApplicationStates::State::THERMOCOUPLE_ERROR);
  });
  this->_thermocouple.setup();
}

double App::celsiusToFarenheit(double celsius) {
  return (celsius * 9.0) / 5.0 + 32;
}

void App::splashScreen() {
  auto splashFile = SPIFFS.open(App::SPLASH_PATH);
  String splashBuffer;
  if (splashFile) {
    while (splashFile.available()) {
      splashBuffer += splashFile.readString();
    }
    Serial.println(splashBuffer);
  }
}

void App::process() {
  this->_statusLED.process();
  if (this->_state == ApplicationStates::State::FATAL_ERROR) {
    return;
  }
  this->_thermocouple.process();
  this->_wifi.process();
  this->_resetButton.read();
  if (this->_resetButton.pressedFor(3000)) {
    this->_log->notice(F("User has pressed the reset button!"));
    this->_wifi.forgetSSID();
    ESP.restart();
  }
}

String App::formatDateTime(tm *time, const char *format) {
  char buffer[64];
  strftime(buffer, sizeof(buffer), format, time);
  return buffer;
}

}