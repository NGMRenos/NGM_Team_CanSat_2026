#include "UVSensor.hpp"

UVSensor::UVSensor(uint8_t pin) : _pin(pin), _raw(0), _voltage(0.0f), _intensity(0.0f) {}

bool UVSensor::begin() {
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(_pin, INPUT);
  return true;
}

void UVSensor::update() {
  long sum = 0;
  const int samples = 16;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(_pin);
    delay(5);
  }
  _raw = sum / samples;
  _voltage = _raw * (3.3f / 4095.0f);
  _intensity = _voltage * 307.0f;
}

int UVSensor::raw() { return _raw; }
float UVSensor::voltage() { return _voltage; }
float UVSensor::intensity() { return _intensity; }
