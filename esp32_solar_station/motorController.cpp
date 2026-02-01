#include <Arduino.h>
#include "motorController.h"

MotorController::MotorController(uint8_t servoPin, uint8_t basePin, double kServo) {
  _basePin = basePin;
  _servoPin = servoPin;
  _kServo = kServo;

  _servoPos = zeroDC;
  _baseSpeed = 0;
  _servoSpeed = 0;
}

// Inizializzazione Hardware
void MotorController::begin() {
  ledcAttach(_servoPin, pwmFreq, pwmRes);
  ledcAttach(_basePin, pwmFreq, pwmRes);
  stopAll();
  t0 = millis();
}


void MotorController::setBaseSpeed(int newSpeed) {
  _baseSpeed = constrain(newSpeed, -100, 100);
}
void MotorController::setServoSpeed(int newSpeed) {
  _servoSpeed = constrain(newSpeed, -100, 100);
}

void MotorController::moveBase() {
  ledcWrite(_basePin, map(_baseSpeed, -100, 100, minDC, maxDC));  // maps to 12 instead of 10 to cap the speed of the base
}
void MotorController::moveServo() {
  uint64_t t1 = micros();
  double deltaT = (t1 - t0) * 0.001;    // milliseconds
  t0 = t1;  // save for the next cycle

  _servoPos += (double)(_servoSpeed) * deltaT * _kServo;
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
void MotorController::moveAuto(int baseError, int servoError) {
  int tempBaseSpeed = baseError * _kpBase;
  int tempServoSpeed = servoError * _kpServo;

  tempBaseSpeed = (abs(tempBaseSpeed) <= 10) ? 0 : tempBaseSpeed;
  tempServoSpeed = (abs(tempServoSpeed) <= 10) ? 0 : tempServoSpeed;
  setBaseSpeed(tempBaseSpeed);
  setServoSpeed(tempServoSpeed);

  Serial.printf("baseError: %d \t servoError: %d \t |   baseSpeed: %d \t servoSpeed: %d \t \n", baseError, servoError, _baseSpeed, _servoSpeed);

  moveBase();
  moveServo();
}

void MotorController::tune(double kpBase, double kpServo) {
  _kpBase = kpBase;
  _kpServo = kpServo;
}