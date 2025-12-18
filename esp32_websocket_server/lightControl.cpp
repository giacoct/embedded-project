#include <Arduino.h>
#include "lightControl.h"

LightControl::LightControl(uint8_t _pin)
  : LightControl(_pin, 2048, 30) {
  // delegated constructor
}

LightControl::LightControl(uint8_t _pin, uint16_t _baseline)
  : LightControl(_pin, _baseline, 30) {
  // delegated constructor
}

LightControl::LightControl(uint8_t _pin, uint16_t _baseline, uint16_t _nReads) {
  pin = _pin;
  baseline = _baseline;
  nReads = _nReads;

  reads = new int[nReads];
  index = 0;
  fullBuffer = false;
}

LightControl::~LightControl() {
  delete[] reads;
}

void LightControl::begin() {
  pinMode(pin, INPUT);
}

void LightControl::sample() {
  reads[index] = analogRead(pin);
  index++;

  if (index >= nReads) {
    index = 0;
    fullBuffer = true;
  }
}

int LightControl::read() {
  unsigned long sum = 0;
  // buffer not full
  uint16_t count = (fullBuffer) ? nReads : index;
  // buffer empty
  if (count == 0) return 0;

  for (uint16_t i = 0; i < count; i++) {
    sum += reads[i];
  }

  return (1000 * sum) / (count * baseline);
}