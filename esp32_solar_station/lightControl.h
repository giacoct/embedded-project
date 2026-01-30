#ifndef __LIGHTCONTROL_H__
#define __LIGHTCONTROL_H__

#include <Arduino.h>

class LightControl {
private:
  uint8_t pin;
  float gain;
  float offset;

  static constexpr int nReads = 50;    // mobile average window
  int reads[nReads];

  uint16_t index;
  bool fullBuffer;

public:
  LightControl(uint8_t _pin, uint16_t _gain, uint16_t _offset);
  void begin();
  int read();

};

#endif