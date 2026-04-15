#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>  // ★ BME280 lib!

class BME280Sensor {  // ★ Όνομα class BME280Sensor
public:
    BME280Sensor();
    bool begin();
    void update();
    float temperature();
    float pressure();
    float humidity();  // ★ ΝΕΟ!
    bool isAvailable();

private:
    Adafruit_BME280 bme;
    bool initialized = false;
    float lastTemp = 0, lastPressure = 0, lastHumidity = 0;  // ★ Humidity
    uint32_t lastUpdate = 0;
    const uint32_t updateIntervalMs = 2000;
};
