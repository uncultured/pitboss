#pragma once

#include <Print.h>
#include <ArduinoLog.h>
#include <PitBoss/Config.h>
#include <PitBoss/StatefulLED.h>
#include <PitBoss/Stateful.h>
#include <FS.h>
#include <Adafruit_MAX31855.h>
#include <vector>
#include <map>
#include <functional>
#include <ESPAsyncWebServer.h>
#include <jled.h>
#include <JC_Button_ESP.h>
#include <ctime>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <PitBoss/Process.h>
#include <PitBoss/StatefulWiFi.h>
#include "StatefulThermocouple.h"
#include "Logger.h"
#include "StatefulDisplay.h"
#include "AsyncUDP.h"

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

  static const int SCREEN_TIMEOUT_MS = 30 * 1000;
  static const int DISPLAY_WIDTH = 128;
  static const int DISPLAY_HEIGHT = 32;
  static const int DISPLAY_I2C_ADDRESS = 0x3C;

  static const int SHORT_PRESS_MS = 2 * 1000;
  static const int LONG_PRESS_MS = 10 * 1000;
  static const gpio_num_t POWER_BUTTON_PIN = GPIO_NUM_0;
  static const gpio_num_t POWER_LED_PIN = GPIO_NUM_4;

  static const gpio_num_t THERMOCOUPLE_CS_PIN = GPIO_NUM_5;
  static const int THERMOCOUPLE_STARTUP_DELAY_MS = 100;

  constexpr static const char* SPLASH_PATH = "/splash.txt";
  constexpr static const char* DEFAULT_CONFIG_FILE_PATH = "/config.json";
  static const int SERVER_PORT = 80;

  Config _config;
  StatefulWiFi _wifi;
  StatefulThermocouple _thermocouple;
  AsyncWebServer _webServer;
  StatefulDisplay _display;
  Button _button;
  JLed _powerLED;
  boolean _sleepEnable = false;
  AsyncUDP _udp;
 public:
  void process() override;
  void setup() override;

  App() :
    Logger(&Log),
    _config(),
    _wifi(&Log, _config.logLevel > LOG_LEVEL_SILENT, _config.wifiCountry, "pitboss-"),
    _thermocouple(&Log, THERMOCOUPLE_STARTUP_DELAY_MS, _config.thermocoupleReadInterval, THERMOCOUPLE_CS_PIN),
    _webServer(SERVER_PORT),
    _display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, DISPLAY_I2C_ADDRESS, SCREEN_TIMEOUT_MS),
    _button(POWER_BUTTON_PIN),
    _powerLED(POWER_LED_PIN)
  {}

 protected:
  bool initSPIFFS();
  static void splashScreen();
  void initLog();
  bool initConfig();
  void initButton();
  void initWebServer();
  void initThermocouple();
  void initNtp();
  void initWifi();

};

}
