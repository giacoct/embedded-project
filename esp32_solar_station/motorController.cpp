#include <Arduino.h>
#include "motorController.h"

MotorController::MotorController(uint8_t servoPin, uint8_t basePin, float kServo) {
  // pins
  _basePin = basePin;
  _servoPin = servoPin;
  // control
  _kServo = kServo;
  _servoPos = (maxDC + minDC) / 2.0;
  _baseSpeed = 0;
  _servoSpeed = 0.0;
}

// Inizializzazione Hardware
void MotorController::begin() {
  ledcAttach(_servoPin, pwmFreq, pwmRes);
  ledcAttach(_basePin, pwmFreq, pwmRes);
  stopAll();
  t0 = millis();
}


void MotorController::setBaseSpeed(int newSpeed) {
  _baseSpeed = constrain(newSpeed, -10, 10);
}
void MotorController::setServoSpeed(int newSpeed) {
  _servoSpeed = constrain(newSpeed, -10, 10);
}

void MotorController::moveBase() {
  ledcWrite(_basePin, map(_baseSpeed, -12, 12, minDC, maxDC));  // maps to 12 instead of 10 to cap the speed of the base
}
void MotorController::moveServo() {
  uint64_t t1 = millis();
  t0 = t1;  // save for the next cycle
  double deltaT = (double)(t1 - t0);

  _servoPos = _servoPos + (_servoSpeed * deltaT * _kServo);
  _servoPos = constrain(_servoPos, minDC, maxDC);
  ledcWrite(_servoPin, (int)_servoPos);
}

void MotorController::stopBase() {
  ledcWrite(_basePin, (maxDC + minDC) / 2);
  _baseSpeed = 0;
}
void MotorController::stopServo() {
  _servoSpeed = 0;
}
void MotorController::stopAll() {
  stopBase();
  stopServo();
}


// automatic control
void MotorController::moveAuto(double baseError, double servoError) {
  // ---------- move base ----------
  double outBase = baseError * kpBase;
  outBase += (minDC + maxDC) / 2;              // shift the PID center from 0 to the midpoint between minDC and maxDC
  outBase = constrain(outBase, minDC, maxDC);  // security measure

  // ---------- move servo ----------
  double outServo = servoError * kpServo;
  outServo += (minDC + maxDC) / 2;               // shift the PID center from 0 to the midpoint between minDC and maxDC
  outServo = constrain(outServo, minDC, maxDC);  // security measure

  // ---------- apply movement ----------
  Serial.printf(" P: %f \tI: %f \tD: %f \ttime: %f \tOutServo: %f", servoErrors[0], servoErrors[1], servoErrors[2], elapsedTime, outServo);
  ledcWrite(_basePin, (int)outBase);
  ledcWrite(_servoPin, (int)outServo);
}

void MotorController::tune(double kpBase, double kpServo) {
  _kpBase = kpBase;
  _kpServo = kpServo;
}