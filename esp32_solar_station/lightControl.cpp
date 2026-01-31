#include "lightControl.h"
#include <Arduino.h>

LightControl::LightControl(uint8_t pin, float gain) {
  _pin = pin;
  _gain = gain;
  _avg = 0.0;
  _init = false;
}

void LightControl::begin() {
  pinMode(_pin, INPUT);
}

void LightControl::update() {
  int raw = analogRead(_pin);

  if (!_init) {
    _avg = raw;
    _init = true;
    return;
  }

  float delta = raw - _avg;
  _avg += ALPHA * delta;
}

unsigned int LightControl::read() {
  return (int)(_gain * _avg + 0.5f);
}
