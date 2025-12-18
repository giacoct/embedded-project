#ifndef __LIGHTCONTROL_H__
#define __LIGHTCONTROL_H__

#include <Arduino.h>

class LightControl {
private:
  uint8_t pin;
  uint16_t nReads;

  int *reads;  // dynamic array
  uint16_t index;
  uint16_t baseline;
  bool fullBuffer;
public:
  LightControl(uint8_t _pin);
  LightControl(uint8_t _pin, uint16_t _baseline);
  LightControl(uint8_t _pin, uint16_t _baseline, uint16_t _nReads);
  ~LightControl();
  void begin();
  void sample();
  int read();
};

#endif