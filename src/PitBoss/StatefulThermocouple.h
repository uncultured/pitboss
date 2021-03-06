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
  unsigned long _lastReading = 0;
  double _currentColdJunction = 0;
  double _currentHotJunction = 0;
  unsigned long _startupDelay;
  unsigned long _readInterval;
  Adafruit_MAX31855 _thermocouple;
 public:
  StatefulThermocouple(Logging* log, unsigned long startupDelay, unsigned long readInterval, int csPin) :
    Logger(log),
    _startupDelay(startupDelay),
    _readInterval(readInterval),
    _thermocouple(csPin)
  {}

  StatefulThermocouple(Logging* log, unsigned long startupDelay, unsigned long readInterval, int csPin, int clkPin, int misoPin) :
    Logger(log),
    _startupDelay(startupDelay),
    _readInterval(readInterval),
    _thermocouple(csPin, clkPin, misoPin)
  {}

  void setup() override {
    this->_lastReading = millis();
    this->_thermocouple.begin();
    this->_log->notice(F("Thermocouple initialized. Waiting %d milliseconds for stabilization before verifying operation."), this->_startupDelay);
    this->_lastReading = millis() + this->_startupDelay;
  }

  void process() override {
    auto now = millis();
    if (millis() - this->_lastReading > this->_readInterval) {
      this->_lastReading = now;
      if (this->readThermocouple(this->_currentColdJunction, this->_currentHotJunction)) {
        if (this->_state != StatefulThermocoupleStates::State::READY) {
          this->setState(StatefulThermocoupleStates::State::READY);
        }
      } else {
        if (this->_state != StatefulThermocoupleStates::State::ERROR) {
          this->setState(StatefulThermocoupleStates::State::ERROR);
        }
      }
    }
  }

  bool readThermocouple(double & coldJunction, double & hotJunction) {
    coldJunction = this->_thermocouple.readInternal();
    if (isnan(coldJunction)) {
      this->_log->error(F("Unable to read cold junction temperature. Is the MAX31855 connected correctly?"));
      this->setState(StatefulThermocoupleStates::State::ERROR);
      return false;
    }
    hotJunction = this->_thermocouple.readCelsius();
    if (isnan(hotJunction)) {
      this->_log->error(F("Unable to read hot junction temperature. Did you plug the thermocouple in correctly?"));
      this->setState(StatefulThermocoupleStates::State::ERROR);
      return false;
    }
    return true;
  }

  void getTemperatures(double & coldJunction, double & hotJunction) const {
    coldJunction = this->_currentColdJunction;
    hotJunction = this->_currentHotJunction;
  }

};

}