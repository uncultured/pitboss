#pragma once

#include "Stateful.h"
#include "Process.h"
#include "Logger.h"
#include <WiFi.h>
#include <WiFiManager.h>

namespace PitBoss {

namespace StatefulWiFiStates {

enum State {
  ERROR,
  DISCONNECTED,
  PROVISIONING,
  CONNECTED,
};

}

class StatefulWiFi :
  public Stateful<StatefulWiFiStates::State>,
  public Process,
  public Logger
{
 protected:
  bool _debug;
  wifi_country_t _wifiCountry;
  const char * _ssidPrefix;
  WiFiManager _wifiManager;
  bool _wifiConnected = false;
 public:

  StatefulWiFi(Logging* log, bool debug, wifi_country_t wifiCountry, const char * ssidPrefix) :
    Logger(log),
    _debug(debug),
    _wifiCountry(wifiCountry),
    _ssidPrefix(ssidPrefix),
    _wifiManager()
  {}

  virtual void setup() {
    if (!WiFiClass::mode(WIFI_MODE_STA)) {
      this->_log->fatal(F("Unable to initialize WiFi."));
      this->setState(StatefulWiFiStates::State::ERROR);
    }
    this->_wifiManager.setDebugOutput(this->_debug);
    this->_wifiManager.setCountry(this->_wifiCountry.cc);
    this->_wifiManager.setConfigPortalBlocking(false);
    if (this->_wifiManager.getWiFiIsSaved()) {
      this->setState(StatefulWiFiStates::State::DISCONNECTED);
    } else {
      this->setState(StatefulWiFiStates::State::PROVISIONING);
    }
    this->_wifiManager.autoConnect((String(F("pitboss-")) + String(WIFI_getChipId(), HEX)).c_str());
  }

  virtual void process() {
    this->_wifiManager.process();
    if (WiFi.isConnected()) {
      if (!this->_wifiConnected) {
        this->_wifiConnected = true;
        this->setState(StatefulWiFiStates::State::CONNECTED);
      }
    } else {
      if (this->_wifiConnected) {
        this->_wifiConnected = false;
        this->setState(StatefulWiFiStates::State::DISCONNECTED);
      }
    }
  }

  int getSignalStrength() {
    return this->_wifiManager.getRSSIasQuality(WiFi.RSSI());
  }

  String getSSID() {
    return this->_wifiManager.getWiFiSSID();
  }

  void forgetSSID() {
    this->_wifiManager.resetSettings();
  }

};

}