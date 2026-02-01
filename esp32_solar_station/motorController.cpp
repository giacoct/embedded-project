#include <Arduino.h>
#include "motorController.h"

MotorController::MotorController(uint8_t servoPin, uint8_t basePin, double kServo) {
  _servoPin = servoPin;
  _basePin = basePin;
  _kServo = kServo;

  _servoPos = zeroDC;
  _baseSpeed = 0;
  _servoSpeed = 0;
}

void MotorController::begin() {
  ledcAttach(_servoPin, pwmFreq, pwmRes);
  ledcAttach(_basePin, pwmFreq, pwmRes);
  stopMotors();
  t0 = millis();
}



void MotorController::setServoSpeed(int newSpeed) {
  _servoSpeed = constrain(newSpeed, -100, 100);
}
void MotorController::setBaseSpeed(int newSpeed) {
  _baseSpeed = constrain(newSpeed, -100, 100);
}

void MotorController::moveMotors() {
  // ----- move servo -----
  uint64_t t1 = micros();
  double deltaT = (t1 - t0) * 0.001;  // milliseconds
  t0 = t1;                            // save for the next cycle

  _servoPos += (double)(_servoSpeed)*deltaT * _kServo;
  _servoPos = constrain(_servoPos, minDC, maxDC);

  ledcWrite(_servoPin, (int)_servoPos);

  // ----- move base -----
  ledcWrite(_basePin, map(_baseSpeed, -100, 100, minDC, maxDC));  // maps to 12 instead of 10 to cap the speed of the base
}

void MotorController::stopMotors() {
  // ----- stop servo  -----
  _servoSpeed = 0;

  // ----- stop base  -----
  ledcWrite(_basePin, (maxDC + minDC) / 2);
  _baseSpeed = 0;
}



int MotorController::mapWithDeadZone(int x, int inMin, int inMax, int outMin, int outMax, int deadZone) {
  deadZone /= 2;  // I need only half the value of the deadZone
  if (x < -deadZone) {
    return map(x, inMin, -deadZone, outMin, 0);
  } else if (x > deadZone) {
    return map(x, deadZone, inMax, 0, outMax);
  } else {
    return 0;
  }
}

void MotorController::moveAuto(int baseError, int servoError) {
  // ----- auto-move servo -----
  servoError *= kpServo;
  setServoSpeed(mapWithDeadZone(servoError, -4095, 4095, -100, 100, deadzone));

  // ----- auto-move base -----
  baseError *= kpBase;
  if (_servoPos - zeroDC > 0) baseError = -baseError;
  setBaseSpeed(mapWithDeadZone(baseError, -4095, 4095, -100, 100, deadzone));

  // ----- apply movements -----
  moveMotors();
}

