#include "lightControl.h"
#include <Arduino.h>

LightControl::LightControl(uint8_t _pin, uint16_t _gain, uint16_t _offset) {
  pin = _pin;
  gain = _gain;
  offset = _offset;

  fullBuffer = false;
  index = 0;
}

void LightControl::begin() {
  pinMode(pin, INPUT);
}

int LightControl::read() {
  reads[index] = analogRead(pin);
  index++;

  if (index >= nReads) {
    index = 0;
    fullBuffer = true;
  }

  // avg
  int count = (fullBuffer) ? nReads : index;  // buffer not full
  if (count == 0) return 0;                   // buffer empty

  unsigned long sum = 0;
  for (int i = 0; i < count; i++) {
    sum += reads[i];
  }

  float avg = (float)sum / count;
  float norm = (avg - offset) / gain;

  return (int)(norm);
}