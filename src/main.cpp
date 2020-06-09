#include <Arduino.h>
#include <PitBoss/App.h>

using namespace PitBoss;

App* pitboss;

void setup() {
  pitboss = new App();
  pitboss->setup();
}

void loop() {
  pitboss->process();
}
