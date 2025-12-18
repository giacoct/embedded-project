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
double MotorController::computePID(double error) {
  currentTime = millis();
  elapsedTime = (double)(currentTime - previousTime);

  cumError += error * elapsedTime;                    // integrative 
  rateError = (error - lastError) / elapsedTime;      // derivative

  double out = kp * error + ki * cumError + kd * rateError;

  // prevent integral wind-up
  out = constrain(out, 0, 255);  

  lastError = error;
  previousTime = currentTime;

  return out;
}

void MotorController::tunePID(double _kp, double _ki, double _kd) {
  kp = _kp;
  ki = _ki;
  kd = _kd;
}