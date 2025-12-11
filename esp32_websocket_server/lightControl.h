#ifndef __LIGHTCONTROL_H__
#define __LIGHTCONTROL_H__

#include <Arduino.h>

class LightControl {
private:
  uint8_t pin;
  uint8_t nReads;
  unsigned int reads[30];
  unsigned int index;
  bool fullBuffer;
public:
  LightControl(uint8_t _pin);
  LightControl(uint8_t _pin, uint8_t _nReads);
  ~LightControl();
  void begin();
  void addRead();
  int getAvg();
};

#endif