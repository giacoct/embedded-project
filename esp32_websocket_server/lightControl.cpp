#include <Arduino.h>
#include "lightControl.h"

LightControl::LightControl(uint8_t _pin) {
  LightControl(_pin, 30);
}

LightControl::LightControl(uint8_t _pin, uint8_t _nReads) {
  pin = _pin;
  nReads = _nReads;
  index = 0;
  fullBuffer = false;
  // reads = new unsigned int[nReads];
}

LightControl::~LightControl() {
  // delete[] reads;
}

void LightControl::begin() {
  pinMode(pin, INPUT_PULLUP);
}

void LightControl::addRead() {
  int tmp = analogRead(pin);
  reads[index] = tmp;
  Serial.printf("valore letto %d \n",tmp);
  index++;

  if (index >= nReads) {
    index = 0;
    fullBuffer = true;
  }
}

int LightControl::getAvg() {
  unsigned long sum = 0;
  // buffer not full
  uint8_t count = fullBuffer ? nReads : index;
  // buffer empty
  if (count == 0) return 0;
  
  for (uint8_t i = 0; i < count; i++) {
    sum += reads[i];
  }
  
  return sum / count;
}