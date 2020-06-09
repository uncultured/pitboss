#pragma once

#include "Process.h"
#include "Logger.h"
#include <PitBoss/Stateful.h>
#include <Adafruit_MAX31855.h>
namespace PitBoss {

namespace StatefulThermocoupleStates {

enum State {
  ERROR,
  READY
};

}

class StatefulThermocouple :
  public Stateful<StatefulThermocoupleStates::State>,
  public Process,
  public Logger
{
 protected:
  int _lastReading = 0;
  double _currentColdJunction = 0;
  double _currentHotJunction = 0;
  int _startupDelay;
  int _readInterval;
  Adafruit_MAX31855 _thermocouple;
 public:
  StatefulThermocouple(Logging* log, int startupDelay, int readInterval, int csPin) :
    Logger(log),
    _startupDelay(startupDelay),
    _readInterval(readInterval),
    _thermocouple(csPin)
  {}

  StatefulThermocouple(Logging* log, int startupDelay, int readInterval, int csPin, int clkPin, int misoPin) :
    Logger(log),
    _startupDelay(startupDelay),
    _readInterval(readInterval),
    _thermocouple(csPin, clkPin, misoPin)
  {}

  virtual void setup() {
    this->_lastReading = millis();
    this->_thermocouple.begin();
    this->_log->notice(F("Thermocouple initialized. Waiting %d milliseconds for stabilization before verifying operation."), this->_startupDelay);
    double coldJunction;
    double hotJunction;
    if (!this->readThermocouple(coldJunction, hotJunction)) {
      this->_log->notice(F("Thermocouple initialization failed."), this->_startupDelay);
      this->setState(StatefulThermocoupleStates::State::ERROR);
    }
  }

  virtual void process() {
    int currentMillis = millis();
    if (currentMillis - this->_lastReading > this->_readInterval) {
      this->_lastReading = currentMillis;
      if (this->readThermocouple(this->_currentColdJunction, this->_currentHotJunction)) {
        if (this->_state == StatefulThermocoupleStates::State::ERROR) {
          this->setState(StatefulThermocoupleStates::State::READY);
        }
      } else {
        if (this->_state == StatefulThermocoupleStates::State::READY) {
          this->setState(StatefulThermocoupleStates::State::ERROR);
        }
      }
    }
  }

  bool readThermocouple(double & coldJunction, double & hotJunction) {
    coldJunction = this->_thermocouple.readInternal();
    if (isnan(coldJunction)) {
      this->_log->error(F("Unable to read cold junction temperature. Is the MAX31855 connected correctly?"));
      return false;
    }
    hotJunction = this->_thermocouple.readCelsius();
    if (isnan(hotJunction)) {
      this->_log->error(F("Unable to read hot junction temperature. Did you plug the thermocouple in correctly?"));
      return false;
    }
    return true;
  }

};

}