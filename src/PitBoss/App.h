#pragma once

#include <Print.h>
#include <ArduinoLog.h>
#include <PitBoss/Config.h>
#include <PitBoss/StatefulLED.h>
#include <PitBoss/Stateful.h>
#include <FS.h>
#include <Adafruit_MAX31855.h>
#include <Wire.h> // Adafruit library refuses to compile without this
#include <vector>
#include <map>
#include <functional>
#include <ESPAsyncWebServer.h>
#include <jled.h>
#include <JC_Button_ESP.h>
#include <time.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <PitBoss/Process.h>
#include <PitBoss/StatefulWiFi.h>
#include "StatefulThermocouple.h"
#include "Logger.h"

namespace PitBoss {

namespace ApplicationStates {

enum State {
  BOOTING,
  PROVISIONING,
  DISCONNECTED,
  READY,
  FATAL_ERROR,
  THERMOCOUPLE_ERROR,
};

}

class App :
  public Stateful<ApplicationStates::State>,
  public Process,
  public Logger
{
 protected:
  static const int BAUD_RATE = 115200;

  static const int STATUS_LED_RED = 25;
  static const int STATUS_LED_GREEN = 26;
  static const int STATUS_LED_BLUE = 27;

  static const int RESET_BUTTON = 33;

  static const int THERMOCOUPLE_CS = 5;
  static const int THERMOCOUPLE_STARTUP_DELAY_MS = 100;

  constexpr static const char* SPLASH_PATH = "/splash.txt";
  constexpr static const char* DEFAULT_CONFIG_FILE_PATH = "/config.json";
  static const int SERVER_PORT = 80;

  Config _config;
  StatefulWiFi _wifi;
  StatefulLED _statusLED;
  StatefulThermocouple _thermocouple;
  AsyncWebServer _webServer;
  Button _resetButton;
 public:
  constexpr static const char * TIME_FORMAT_ISO_8601 = "%Y-%m-%dT%H:%M:%SZ";
  static String formatDateTime(tm *time, const char * format = TIME_FORMAT_ISO_8601);

  static double celsiusToFarenheit(double celsius);

  virtual void process();
  virtual void setup();

  App() :
    Logger(&Log),
    _config(),
    _wifi(&Log, _config.logLevel > LOG_LEVEL_SILENT, _config.wifiCountry, "pitboss-"),
    _statusLED(STATUS_LED_RED, STATUS_LED_GREEN, STATUS_LED_BLUE),
    _thermocouple(&Log, THERMOCOUPLE_STARTUP_DELAY_MS, _config.thermocoupleReadInterval, THERMOCOUPLE_CS),
    _webServer(SERVER_PORT),
    _resetButton(RESET_BUTTON)
  {}

 protected:
  bool initSPIFFS();
  static void splashScreen();
  void initStatusLED();
  void initLog();
  bool initConfig();
  void initWebServer();
  void initThermocouple();
  void initNtp();
  void initWifi();

};

}
