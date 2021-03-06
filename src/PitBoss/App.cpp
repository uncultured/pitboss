#include <SPIFFS.h>
#include <PitBoss/App.h>
#include <PitBoss/TimeHelper.h>
#include <PitBoss/TemperatureHelper.h>

namespace PitBoss {

void App::setup() {
  Serial.begin(App::BAUD_RATE);
  this->_display.setup();
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
  this->initButton();
  this->initWebServer();
  this->initNtp();
  this->initWifi();
  this->initThermocouple();
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

void App::initButton() {
  this->_button.begin();
  this->_display.onState(StatefulDisplayStates::State::ON, [this](){
    this->_powerLED.Stop();
  });
  this->_display.onState(StatefulDisplayStates::State::OFF, [this](){
    this->_powerLED
      .Reset()
      .Breathe(1000)
      .Forever()
      .DelayAfter(5000);
  });
  esp_sleep_enable_ext0_wakeup(App::POWER_BUTTON_PIN,0);
}

void App::initWebServer() {
  this->_webServer.onNotFound([](AsyncWebServerRequest *request){
    Log.notice("404");
  });
  this->_webServer.on("/", [](AsyncWebServerRequest *request){
    request->redirect("/temperature");
  });
  this->_webServer.on("/temperature", HTTP_GET, [this](AsyncWebServerRequest *request){
    double coldJunction = 0;
    double hotJunction = 0;
    if (!this->_thermocouple.readThermocouple(coldJunction, hotJunction)) {
      request->send(500);
      return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    StaticJsonDocument<256> json;
    json["time"] = getTime();
    json["coldJunction"] = celsiusToFarenheit(coldJunction);
    json["hotJunction"] = celsiusToFarenheit(hotJunction);
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
      configTime(this->_config.gmtOffset, this->_config.dstOffset, this->_config.ntpServer.c_str(), Config::DEFAULT_NTP_SERVER, WiFi.gatewayIP().toString().c_str());
    } else {
      configTime(this->_config.gmtOffset, this->_config.dstOffset, Config::DEFAULT_NTP_SERVER, WiFi.gatewayIP().toString().c_str());
    }
    this->_log->notice(F("Got time: %s"), getTime().c_str());
    this->_display.startTime();
  });
}

void App::initWifi() {
  this->_wifi.onState(StatefulWiFiStates::State::CONNECTED, [this](){
    this->setState(ApplicationStates::State::READY);
    this->_udp.connect(WiFi.localIP(), 8888);
    this->_display.updateWiFi(StatefulWiFiStates::State::CONNECTED,
                              WiFi.localIP(),
                              this->_wifi.getSSID(),
                              this->_wifi.getSignalStrength());
  });
  this->_wifi.onState(StatefulWiFiStates::State::DISCONNECTED, [this](){
    this->setState(ApplicationStates::State::DISCONNECTED);
    this->_display.updateWiFi(StatefulWiFiStates::State::DISCONNECTED);
  });
  this->_wifi.onState(StatefulWiFiStates::State::PROVISIONING, [this](){
    this->setState(ApplicationStates::State::PROVISIONING);
    this->_display.updateWiFi(StatefulWiFiStates::State::PROVISIONING);
  });
  this->_wifi.onState(StatefulWiFiStates::State::ERROR, [this](){
    this->setState(ApplicationStates::State::FATAL_ERROR);
    this->_display.updateWiFi(StatefulWiFiStates::State::ERROR);
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
    this->_display.updateThermocouple(StatefulThermocoupleStates::State::ERROR);
  });
  this->_thermocouple.setup();
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
  if (this->_state == ApplicationStates::State::FATAL_ERROR) {
    return;
  }
  this->_powerLED.Update();
  this->_thermocouple.process();
  this->_wifi.process();
  if (this->_wifi.getState() == StatefulWiFiStates::State::CONNECTED) {
    this->_display.updateWiFi(
      StatefulWiFiStates::State::CONNECTED,
      WiFi.localIP(),
      this->_wifi.getSSID(),
      this->_wifi.getSignalStrength()
    );
    this->_udp.broadcast("wazzap");
  }
  if (this->_thermocouple.getState() == StatefulThermocoupleStates::State::READY) {
    double coldJunction;
    double hotJunction;
    this->_thermocouple.getTemperatures(coldJunction, hotJunction);
    this->_display.updateThermocouple(
      StatefulThermocoupleStates::State::READY,
      coldJunction,
      hotJunction
    );
  }
  this->_display.process();
  this->_button.read();
  if (this->_button.isPressed()) {
    if (this->_display.getState() == StatefulDisplayStates::State::OFF) {
      this->_display.wakeup();
    }
  }
  if (this->_button.pressedFor(App::SHORT_PRESS_MS)) {
    this->_sleepEnable = true;
  }
  if (this->_sleepEnable && this->_button.isReleased()) {
    this->_sleepEnable = false;
    this->_log->notice(F("Going to sleep."));
    this->_display.sleep();
    esp_deep_sleep_start();
  }
  if (this->_button.pressedFor(App::LONG_PRESS_MS)) {
    this->_log->notice(F("Resetting."));
    this->_wifi.forgetSSID();
    ESP.restart();
  }
}

}