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

  // pins
  uint8_t _servoPin, _basePin;

  // base & servo control
  int _servoSpeed, _baseSpeed;
  double _servoPos, _kServo;
  uint64_t t0;

  // auto movement
  int mapWithDeadZone(int x, int inMin, int inMax, int outMin, int outMax, int deadzone);

public:
  MotorController(uint8_t _servoPin, uint8_t _basePin, double _kServo);
  void begin();

  // manual movement
  void setServoSpeed(int newSpeed);
  void setBaseSpeed(int newSpeed);
  void moveMotors();
  void stopMotors();

  // auto movement
  int deadzone;
  double kpBase, kpServo;
  void moveAuto(int baseError, int servoError);
  // void tune(double kpBase, double kpServo, int deadzone);
};

#endif
