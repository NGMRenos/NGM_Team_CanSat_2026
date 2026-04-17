#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

class SDCard {
public:
    SDCard();
    bool begin();
    bool logMessage(double lat, double lon, uint32_t sats,
        uint8_t h, uint8_t m, uint8_t s,
        uint8_t day, uint8_t month, uint16_t year,
        float temp, float press);
    bool isAvailable();

private:
    SPIClass _spi = HSPI;
    bool _initialized = false;
    const char* _filename = "/loralog.csv";  // ★ /
};
