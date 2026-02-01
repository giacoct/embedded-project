#ifndef __MOTORCONTROLLER_H__
#define __MOTORCONTROLLER_H__

#include <Arduino.h>

class MotorController {
private:
  static constexpr uint8_t pwmFreq = 50;                  // PWM frequency specific to servo motor
  static constexpr uint8_t pwmRes = 10;                   // PWM resolution 2^10 values
  static constexpr uint8_t minDC = 41;                    // real value: minDC = 26
  static constexpr uint8_t maxDC = 111;                   // real value: maxDC = 126
  static constexpr uint8_t zeroDC = (minDC + maxDC) / 2;  // real value: maxDC = 126

  // ----- pins -----
  uint8_t _servoPin, _basePin;

  // ----- base & servo control -----
  double _kServo;
  double _servoPos;
  int _servoSpeed, _baseSpeed;
  uint64_t t0;

  // ----- auto -----
  double _kpBase, _kpServo;

public:
  MotorController(uint8_t _servoPin, uint8_t _basePin, double _kServo);
  void begin();
  // manual movement
  void setBaseSpeed(int newSpeed);
  void setServoSpeed(int newSpeed);
  void moveBase();
  void moveServo();
  void stopBase();
  void stopServo();
  void stopAll();
  // auto movement - proportional
  void moveAuto(int baseError, int servoError);
  void tune(double kpBase, double kpServo);
};

#endif
