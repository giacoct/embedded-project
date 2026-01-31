#ifndef __LIGHTCONTROL_H__
#define __LIGHTCONTROL_H__

#include <Arduino.h>

class LightControl {
private:
  uint8_t _pin;
  float _gain;
  float _avg;
  bool _init;
  
  static constexpr float ALPHA = 0.1;

public:
  LightControl(uint8_t pin, float gain);

  void begin();
  void update();
  unsigned int read();
};

#endif