#include <Arduino.h>
#include "motorController.h"


MotorController::MotorController(uint8_t _servoPin, uint8_t _basePin, float _kServo) {
  servoPos = (maxDutyCycle + minDutyCycle) / 2.0;
  kServo = _kServo;
  baseSpeed = 0;
  servoSpeed = 0.0;
  // pins
  basePin = _basePin;
  servoPin = _servoPin;
  // pid
  currentTime = 0;
  previousTime = currentTime;
}

// Inizializzazione Hardware
void MotorController::begin() {
  // Setup PWM Servo (Y)
  ledcAttach(servoPin, pwmFreq, pwmResolution);
  ledcWrite(servoPin, 50);

  // Setup PWM Base (X)
  ledcAttach(basePin, pwmFreq, pwmResolution);

  t0 = millis();
}


void MotorController::setBaseSpeed(int newSpeed) {
  baseSpeed = constrain(newSpeed, -10, 10);
}

void MotorController::setServoSpeed(int newSpeed) {
  servoSpeed = constrain(newSpeed, -10, 10);
}


void MotorController::moveBase() {
  ledcWrite(basePin, map(baseSpeed, -12, 12, minDutyCycle, maxDutyCycle));  // maps to 12 instead of 10 to cap the speed of the base
}

void MotorController::moveServo() {
  uint64_t t1 = millis();
  uint64_t deltaT_ms = t1 - t0;
  t0 = t1;  // save for the next cycle
  float deltaT = (float)deltaT_ms;

  servoPos = servoPos + (servoSpeed * deltaT * kServo);
  servoPos = constrain(servoPos, minDutyCycle, maxDutyCycle);
  ledcWrite(servoPin, (int)servoPos);
}


// Forced motor stop
void MotorController::stopBase() {
  ledcWrite(basePin, (maxDutyCycle + minDutyCycle) / 2);
  baseSpeed = 0;
}

void MotorController::stopServo() {
  servoSpeed = 0;
}

void MotorController::stopAll() {
  stopBase();
  stopServo();
}


// PID control
void MotorController::moveWithPID(double baseErrorInst, double servoErrorInst) {
  currentTime = micros();
  if (currentTime == previousTime) return;
  double elapsedTime = (currentTime - previousTime) / 1000.0;  // elapsed time in milliseconds

  // ---------- move base ----------
  baseErrors[2] = (baseErrorInst - baseErrors[0]) / elapsedTime;  // derivative
  baseErrors[1] += baseErrorInst * elapsedTime;                   // integrative
  baseErrors[0] = baseErrorInst;                                  // proportional

  double outBase;
  for (int i = 0; i < 3; i++) {
    outBase += baseErrors[i] * baseK[i];
  }
  outBase += (minDutyCycle + maxDutyCycle) / 2;              // shift the PID center from 0 to the midpoint between minDutyCycle and maxDutyCycle
  outBase = constrain(outBase, minDutyCycle, maxDutyCycle);  // prevents integral wind-up
  // setBaseSpeed(outBase);

  // ---------- move servo ----------
  servoErrors[2] = (servoErrorInst - servoErrors[0]) / elapsedTime;  // derivative
  servoErrors[1] += servoErrorInst * elapsedTime;                    // integrative
  servoErrors[0] = servoErrorInst;                                   // proportional

  double outServo;
  for (int i = 0; i < 3; i++) {
    outServo += servoErrors[i] * servoK[i];
  }
  outServo += (minDutyCycle + maxDutyCycle) / 2;               // shift the PID center from 0 to the midpoint between minDutyCycle and maxDutyCycle
  outServo = constrain(outServo, minDutyCycle, maxDutyCycle);  // prevents integral wind-up
  // setServoSpeed(outServo);

  // ---------- apply movement ----------
  int intOutServo = (int)outServo;
  int intOutBase = (int)outBase;
  Serial.printf(" P: %f \tI: %f \tD: %f \ttime: %f \tOutServo: %f", servoErrors[0], servoErrors[1], servoErrors[2], elapsedTime, outServo);
  ledcWrite(basePin, (int) intOutBase);
  ledcWrite(servoPin, (int) intOutServo);


  previousTime = currentTime;
}

void MotorController::tunePID(const double* _baseK, const double* _servoK) {
  for (int i = 0; i < 3; i++) {
    baseK[i] = _baseK[i];
    servoK[i] = _servoK[i];
  }
}