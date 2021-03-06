#pragma once

#include <Adafruit_SSD1306.h>
#include <Fonts/TomThumb.h>
#include <Fonts/FreeSans18pt7b.h>
#include <PitBoss/TimeHelper.h>
#include <PitBoss/TemperatureHelper.h>

namespace PitBoss {

namespace StatefulDisplayStates {

enum State {
  ON,
  OFF
};

}

class StatefulDisplay : public Stateful<StatefulDisplayStates::State>, public Process {
 protected:
  enum WifiScrollDirection {
    LEFT,
    RIGHT
  };
  Adafruit_SSD1306 _display;
  int _i2cAddress;
  unsigned long _lastFrame;
  unsigned long _screenOnAt;
  unsigned long _screenTimeout;
  static const unsigned long FRAME_INTERVAL = 100;

  StatefulWiFiStates::State _wifiState;
  IPAddress _ipAddress;
  String _ssid;
  int _wifiSignalStrength;

  StatefulThermocoupleStates::State _thermocoupleState;
  double _coldJunction;
  double _hotJunction;

  bool _timeReady;

 public:
  StatefulDisplay(int width, int height, TwoWire* wire, int i2cAddress, int screenTimeout = 30000) :
    _display(width, height, wire),
    _i2cAddress(i2cAddress),
    _lastFrame(0),
    _screenOnAt(0),
    _screenTimeout(screenTimeout),
    _wifiState(),
    _ipAddress(INADDR_NONE),
    _ssid(""),
    _wifiSignalStrength(0),
    _thermocoupleState(),
    _coldJunction(),
    _hotJunction(),
    _timeReady(false)
  {}

  void updateWiFi(
    StatefulWiFiStates::State wifiState,
    const IPAddress& ipAddress = INADDR_NONE,
    const String& ssid = "",
    int signalStrength = 0
  ) {
    this->_wifiState = wifiState;
    this->_ipAddress = ipAddress;
    this->_ssid = ssid;
    this->_wifiSignalStrength = signalStrength;
  }

  void updateThermocouple(
    StatefulThermocoupleStates::State thermocoupleState,
    double coldJunction = NAN,
    double hotJunction = NAN
  ) {
    this->_thermocoupleState = thermocoupleState;
    this->_coldJunction = coldJunction;
    this->_hotJunction = hotJunction;
  }

  void startTime() {
    this->_timeReady = true;
  }

  void wakeup() {
    this->setState(StatefulDisplayStates::State::ON);
  }

  void sleep() {
    this->setState(StatefulDisplayStates::State::OFF);
  }

  void setup() override {
    this->_display.begin(SSD1306_SWITCHCAPVCC, this->_i2cAddress);
    this->onState(StatefulDisplayStates::State::ON, [this](){
      if (this->getPreviousState() != this->getState()) {
        Serial.println("screen on");
        this->_screenOnAt = millis();
        this->_display.clearDisplay();
        this->_display.display();
        this->_display.ssd1306_command(SSD1306_DISPLAYON);
      }
    });
    this->onState(StatefulDisplayStates::State::OFF, [this](){
      if (this->getPreviousState() != this->getState()) {
        Serial.println("screen off");
        this->_display.clearDisplay();
        this->_display.ssd1306_command(SSD1306_DISPLAYOFF);
      }
    });
    this->setState(StatefulDisplayStates::State::ON);
  }

  void process() override {
    if (this->getState() == StatefulDisplayStates::State::OFF) {
      return;
    }
    unsigned long now = millis();
    if (this->_screenTimeout != 0 && now - this->_screenOnAt > this->_screenTimeout) {
      this->setState(StatefulDisplayStates::State::OFF);
      return;
    }
    if (now - this->_lastFrame < StatefulDisplay::FRAME_INTERVAL) {
      return;
    }
    this->_display.clearDisplay();
    this->_renderWiFiStatus();
    this->_renderTemperatures();
    this->_renderTime();
    this->_display.display();
    this->_lastFrame = now;
//    if (now - this->_lastFrame > StatefulDisplay::FRAME_INTERVAL) {
//      this->_display.clearDisplay();
//      const auto height = this->_display.height();
//      const auto width = this->_display.width();
//      this->_display.setTextColor(SSD1306_WHITE);
//      this->_display.setFont(&TomThumb);
//      this->_display.setTextSize(1);
//      this->_display.setTextWrap(false);
//      int16_t x, y;
//      uint16_t w, h;
//      switch (this->_wifiState) {
//        case StatefulWiFiStates::State::CONNECTED: {
//          this->_display.getTextBounds(this->_ssid, 0, height, &x, &y, &w, &h);
//          this->_display.setCursor(0, 6);
//          this->_display.print(this->_ipAddress);
//          this->_display.setCursor(0, 12);
//          this->_display.print(this->_ssid);
//          this->_display.setCursor(0, 18);
//          this->_display.printf("Signal: %i%%", this->_wifiSignalStrength);
//          break;
//        }
//        case StatefulWiFiStates::State::DISCONNECTED:
//          this->_display.setCursor(0, 6);
//          this->_display.print("Connecting...");
//          break;
//        case StatefulWiFiStates::State::PROVISIONING:
//          this->_display.setCursor(0, 6);
//          this->_display.print("WiFi config portal active...");
//          break;
//        case StatefulWiFiStates::State::ERROR:
//          this->_display.setCursor(0, 6);
//          this->_display.print("WiFi failed to initialize...");
//          break;
//      }
//
//      this->_display.setFont(&FreeSans18pt7b);
//      this->_display.setCursor(0, 18);
//      this->_display.getTextBounds("72", 0, height, &x, &y, &w, &h);
//      this->_display.setCursor(width - w - 6, 24);
//      this->_display.print("72");
//      this->_display.setFont(&TomThumb);
//      this->_display.setCursor(width - 3, 6);
//      this->_display.print("F");
//      if (now % 2 == 0) {
//        this->_display.setCursor(0, height);
//        this->_display.print('.');
//      }
//      this->_display.display();
//      this->_lastFrame = now;
//    }
  };

 protected:

  void _renderWiFiStatus() {
    this->_display.setTextColor(SSD1306_WHITE);
    this->_display.setFont(&TomThumb);
    this->_display.setTextSize(1);
    this->_display.setTextWrap(false);
    int16_t x, y;
    uint16_t w, h;
    switch (this->_wifiState) {
      case StatefulWiFiStates::State::CONNECTED: {
        this->_display.getTextBounds(this->_ssid, 0, this->_display.height(), &x, &y, &w, &h);
        this->_display.setCursor(0, 6);
        this->_display.print(this->_ipAddress);
        this->_display.setCursor(0, 12);
        this->_display.print(this->_ssid);
        this->_display.setCursor(0, 18);
        this->_display.printf("Signal: %i%%", this->_wifiSignalStrength);
        break;
      }
      case StatefulWiFiStates::State::DISCONNECTED:
        this->_display.setCursor(0, 6);
        this->_display.print("Connecting...");
        break;
      case StatefulWiFiStates::State::PROVISIONING:
        this->_display.setCursor(0, 6);
        this->_display.print("WiFi config portal active...");
        break;
      case StatefulWiFiStates::State::ERROR:
        this->_display.setCursor(0, 6);
        this->_display.print("WiFi failed to initialize...");
        break;
    }
  }

  void _renderTime() {
    if (!this->_timeReady) {
      return;
    }
    this->_display.setCursor(0, this->_display.height());
    this->_display.print(getTime("%F %r"));
  }

  void _renderTemperatures() {
    if (this->_thermocoupleState != StatefulThermocoupleStates::State::READY) {
      return;
    }
    auto height = this->_display.height();
    auto width = this->_display.width();
    auto hotJunction = String(celsiusToFarenheit(this->_hotJunction), 0);
    int16_t x, y;
    uint16_t w, h;
    this->_display.setFont(&FreeSans18pt7b);
    this->_display.setCursor(0, 18);
    this->_display.getTextBounds(hotJunction, 0, height, &x, &y, &w, &h);
    this->_display.setCursor(width - w - 6, 24);
    this->_display.print(hotJunction);
    this->_display.setFont(&TomThumb);
    this->_display.setCursor(width - 3, 6);
    this->_display.print("F");
  }

};

}