#include "BME280.hpp"

BME280Sensor::BME280Sensor() {}

bool BME280Sensor::begin() {
    Wire.begin(45, 46);
    // Quick scan for BME280 addresses (ίδια με BMP: 0x76/0x77)
    uint8_t addrs[] = {0x76, 0x77};
    for (int i = 0; i < 2; i++) {
        if (bme.begin(addrs[i])) {
            Serial.printf("BME280 @0x%02X OK!\n", addrs[i]);
            bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                           Adafruit_BME280::SAMPLING_X2,   // Temp
                           Adafruit_BME280::SAMPLING_X16,  // Pressure
                           Adafruit_BME280::SAMPLING_X1);  // ★ Humidity X1
            initialized = true;
            return true;
        }
    }
    Serial.println("BME280 NOT FOUND");
    return false;
}

void BME280Sensor::update() {
    if (!initialized || (millis() - lastUpdate < updateIntervalMs)) return;
    lastUpdate = millis();
    lastTemp = bme.readTemperature();
    lastPressure = bme.readPressure() / 100.0F;
    lastHumidity = bme.readHumidity();  // ★ ΝΕΟ!
}

float BME280Sensor::temperature() { return lastTemp; }
float BME280Sensor::pressure() { return lastPressure; }
float BME280Sensor::humidity() { return lastHumidity; }  // ★ ΝΕΟ!
bool BME280Sensor::isAvailable() { return initialized; }
