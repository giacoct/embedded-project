#include <Arduino.h>
#include "motorController.h"


MotorController::MotorController(uint8_t _servoPin, uint8_t _basePin, float _kServo) {
  servoPos = (maxDutyCycle + minDutyCycle) / 2.0;
  // base control
  baseSpeed = 0.0;
  servoSpeed = 0.0;
  servoPin = _servoPin;
  basePin = _basePin;
  kServo = _kServo;
}

// Inizializzazione Hardware
void MotorController::begin() {
  // Setup PWM Servo Y
  ledcAttach(servoPin, pwmFreq, pwmResolution);
  ledcWrite(servoPin, (int)servoPos);

  // Setup PWM Base X
  ledcAttach(basePin, pwmFreq, pwmResolution);

  t0 = millis();
}

// Ferma immediatamente i motori
void MotorController::stopBase() {
  ledcWrite(basePin, (maxDutyCycle + minDutyCycle) / 2);
}

// // Funzione helper per mappare mantenendo precisione float prima del cast
// long MotorController::mapFloat(float x, float in_min, float in_max, long out_min, long out_max) {
//   return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
// }

void MotorController::moveServo() {
  uint64_t deltaT = millis() - t0;    // media 40 ms    max 42ms

  servoPos = servoPos + (servoSpeed * (float)deltaT * kServo);
  if (servoPos > maxDutyCycle) servoPos = float(maxDutyCycle);
  if (servoPos < minDutyCycle) servoPos = float(minDutyCycle);
  ledcWrite(servoPin, (int)servoPos);

  Serial.printf("servopos: %f", servoPos);

  t0 = millis();
}

void MotorController::moveBase() {
 ledcWrite(basePin, map(baseSpeed, -10, 10, minDutyCycle, maxDutyCycle));
}


void MotorController::setServoSpeed(float newSpeed) {
  servoSpeed = newSpeed;
  Serial.printf("settata nuova velocità servo: %f\n", servoSpeed);
}

void MotorController::setBaseSpeed(float newSpeed) {
  baseSpeed = newSpeed;
  // Serial.println("settata nuova velocità base");
}