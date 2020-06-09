#pragma once

#include <functional>
#include <map>
#include <vector>

namespace PitBoss {

template<typename T_state>
class Stateful {
 protected:
  T_state _state;
  T_state _previousState;
  std::map<T_state, std::vector<std::function<void(void)>>>
    _stateListeners = std::map<T_state, std::vector<std::function<void()>>>();
 public:
  Stateful &onState(T_state state, const std::function<void(void)> &listener) {
    if (this->_stateListeners.find(state) == this->_stateListeners.end()) {
      std::vector<std::function<void(void)>> listeners;
      this->_stateListeners.insert({state, listeners});
    }
    this->_stateListeners.at(state).push_back(listener);
    return *this;
  }
  T_state getState() {
    return this->_state;
  }
  T_state getPreviousState() {
    return this->_previousState;
  }
 protected:
  void setState(T_state state) {
    this->_previousState = this->_state;
    this->_state = state;
    this->notifyStateListeners();
  }
  virtual void notifyStateListeners() {
    if (this->_stateListeners.find(this->_state) != this->_stateListeners.end()) {
      auto listeners = this->_stateListeners.at(this->_state);
      for (auto &listener : listeners) {
        listener();
      }
    }
  }
};

}