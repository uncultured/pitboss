#pragma once

#include <Print.h>
#include <ArduinoLog.h>
namespace PitBoss {

class Logger {
 private:
  static bool _loggingStarted;
 protected:
  Logging* _log;
 public:
  Logger(Logging* log) :
    _log(log)
  {}
};

}