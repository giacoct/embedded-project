#ifndef __LIGHTCONTROL_H__
#define __LIGHTCONTROL_H__

#include <Arduino.h>

class LightControl {
private:
  uint8_t _pin;
  float _gain;
  float _offset;
  float _alpha;

  float _avg;   // EMA
  bool _init;

public:
  LightControl(uint8_t pin, float gain, float offset);

  void begin();
  void update();
  int read();

};

#endif