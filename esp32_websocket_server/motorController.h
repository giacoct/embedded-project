#ifndef __MOTORCONTROLLER_H__
#define __MOTORCONTROLLER_H__

#include <Arduino.h>

class MotorController {
private:
  // ----- base & servo control -----
  const uint8_t pwmFreq = 50;        // PWM frequency specific to servo motor
  const uint8_t pwmResolution = 10;  // PWM resolution 2^10 values
  const uint8_t minDutyCycle = 26;
  const uint8_t maxDutyCycle = 126;
  float kServo;
  float servoSpeed, servoPos;
  int baseSpeed;
  uint64_t t0;
  // ----- pins -----
  uint8_t servoPin, basePin;
  // ----- pid -----
  // kp, ki, kd
  double servoK[3];
  double baseK[3];
  // cumulative, rate, latest error
  double baseErrors[3];
  double servoErrors[3];
  unsigned long currentTime, previousTime;

public:
  MotorController(uint8_t _servoPin, uint8_t _basePin, float _kServo);
  void begin();
  void setBaseSpeed(int newSpeed);
  void setServoSpeed(float newSpeed);
  void moveBase();
  void moveServo();
  void stopBase();
  void stopServo();
  void stopAll();
  // pid
  void moveWithPID(double baseErrorInst, double servoErrorInst);
  void tunePID(const double* _baseK, const double* _servoK);
};

#endif
