#ifndef ENS160_HPP
#define ENS160_HPP

#include <Arduino.h>
#include <Wire.h>

class ENS160Sensor {
private:
    uint8_t i2cAddress;
    bool available;

public:
    // Constructor με default address 0x53
    ENS160Sensor(uint8_t addr = 0x52);

    bool begin();
    bool isAvailable();
    void update();
    
    // Set environmental data for compensation
    void setEnvData(float tempC, float humidity);

    // Getters
    uint8_t getAQI();
    uint16_t getTVOC();
    uint16_t geteCO2();
    uint16_t getHP0();
    uint16_t getHP1();
    uint16_t getHP2();
    uint16_t getHP3();
};

#endif
