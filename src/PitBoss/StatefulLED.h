#pragma once

#include <jled.h>
#include "Stateful.h"
#include "Process.h"

namespace PitBoss {

namespace StatefulLEDStates {

enum State {
  WAITING,
  ERROR,
  READY
};

}

class StatefulLED : public Stateful<StatefulLEDStates::State>, Process {
 public:

 protected:
  JLed _red;
  JLed _green;
  JLed _blue;

 public:
  StatefulLED(int red, int green, int blue) :
    _red(JLed(red)),
    _green(JLed(green)),
    _blue(JLed(blue))
  {}

  void waiting() {
    this->setState(StatefulLEDStates::State::WAITING);
  }
  void error() {
    this->setState(StatefulLEDStates::State::ERROR);
  }
  void ready() {
    this->setState(StatefulLEDStates::State::READY);
  }

  virtual void setup() {
    this->onState(StatefulLEDStates::State::WAITING, [this]() {
      this->_red.Breathe(900).Forever();
      this->_green.Breathe(1000).Forever();
      this->_blue.Breathe(1100).Forever();
    });
    this->onState(StatefulLEDStates::State::ERROR, [this]() {
      this->_red.On();
      this->_green.Off();
      this->_blue.Off();
    });
    this->onState(StatefulLEDStates::State::READY, [this]() {
      this->_red.Off();
      this->_green.Breathe(5000).Forever();
      this->_blue.Off();
    });
  }

  virtual void process() {
    this->_red.Update();
    this->_green.Update();
    this->_blue.Update();
  }

};

}