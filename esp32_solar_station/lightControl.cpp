#include "lightControl.h"
#include <Arduino.h>

LightControl::LightControl(uint8_t pin, float gain, float offset) {
  _pin = pin;
  _gain = gain;
  _offset = offset;

  _alpha = 0.1;     // 0.05 is more stable; 0.20 is faster but noisy
  _avg = 0.0;
  _init = false;
}

void LightControl::begin() {
  pinMode(_pin, INPUT);
}

void LightControl::update() {
  int raw = analogRead(_pin);

  if (!_init) {
    _avg  = raw;
    _init = true;
  } else {
    _avg = _alpha * raw + (1.0 - _alpha) * _avg;
  }
}

int LightControl::read() {
  float normalized = (_avg - _offset) / _gain;
  return (int)(normalized + 0.5);
}