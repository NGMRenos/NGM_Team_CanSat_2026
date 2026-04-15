#pragma once
#include <Arduino.h>

class UVSensor {
public:
  UVSensor(uint8_t pin = 18);

  bool begin();
  void update();
  int   raw();
  float voltage();
  float intensity();

private:
  uint8_t _pin;
  int _raw;
  float _voltage;
  float _intensity;
};
