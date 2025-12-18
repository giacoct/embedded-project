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
  currentTime = micros()
  previousTime = currentTime;
}

// Inizializzazione Hardware
void MotorController::begin() {
  // Setup PWM Servo (Y)
  ledcAttach(servoPin, pwmFreq, pwmResolution);
  ledcWrite(servoPin, (int)servoPos);

  // Setup PWM Base (X)
  ledcAttach(basePin, pwmFreq, pwmResolution);

  t0 = millis();
}


void MotorController::setBaseSpeed(int newSpeed) {
  baseSpeed = newSpeed;
}

void MotorController::setServoSpeed(float newSpeed) {
  servoSpeed = newSpeed;
}


void MotorController::moveBase() {
  ledcWrite(basePin, map(baseSpeed, -10, 10, minDutyCycle, maxDutyCycle));
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
  double elapsedTime = (double)(currentTime - previousTime);

  // ---------- move base ----------
  baseErrors[0] += baseErrorInst * elapsedTime;                   // integrative
  baseErrors[1] = (baseErrorInst - baseErrors[2]) / elapsedTime;  // derivative
  baseErrors[2] = baseErrorInst;

  double outBase = baseK[0] * baseErrorInst + baseK[1] * baseErrors[0] + baseK[2] * baseErrors[1];
  // outBase = constrain(outBase, 0, 255);  // prevent integral wind-up

  // ---------- move servo ----------
  servoErrors[0] += servoErrorInst * elapsedTime;                    // integrative
  servoErrors[1] = (servoErrorInst - servoErrors[2]) / elapsedTime;  // derivative
  servoErrors[2] = servoErrorInst;

  double outServo = servoK[0] * servoErrorInst + servoK[1] * servoErrors[0] + servoK[2] * servoErrors[1];
  // outServo = constrain(outServo, 0, 255);  // prevent integral wind-up

  // ---------- apply movement ----------
  int intOutServo = (int)outServo;
  int intOutBase = (int)outBase;
  Serial.printf("er0: %f \ter1: %f \ter2: %f \tOutServo: %f \tOutBase: %f \t", baseErrors[0], baseErrors[1], baseErrors[2], outServo, outBase);
  /*ledcWrite(basePin, (int) intOutBase);
  ledcWrite(servoPin, (int) intOutServo);
  */

  previousTime = currentTime;
}

void MotorController::tunePID(const double* _baseK, const double* _servoK) {
  for (int i = 0; i < 3; i++) {
    baseK[i] = _baseK[i];
    servoK[i] = _servoK[i];
  }
}