#ifndef __MOTORCONTROLLER_H__
#define __MOTORCONTROLLER_H__

#include <Arduino.h>

class MotorController {
private:
  // base & servo control
  const uint8_t pwmFreq = 50;        // PWM frequency specific to servo motor
  const uint8_t pwmResolution = 10;  // PWM resolution 2^10 values
  const uint8_t minDutyCycle = 26;
  const uint8_t maxDutyCycle = 126;
  float kServo;
  float servoSpeed, servoPos;
  int baseSpeed;
  uint64_t t0;
  // pins
  uint8_t servoPin, basePin;
  // pid
  double kp, ki, kd;
  unsigned long currentTime, previousTime;
  double elapsedTime;
  double cumError, rateError, lastError;

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
  double computePID(double error);
  void tunePID(double _kp, double _ki, double _kd);
};

#endif
