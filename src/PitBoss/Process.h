#pragma once

namespace PitBoss {

class Process {
 public:
  virtual void setup() = 0;
  virtual void process() = 0;
};

}
