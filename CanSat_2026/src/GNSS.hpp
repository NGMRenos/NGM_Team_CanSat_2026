#pragma once
#include <Arduino.h>
#include "HT_TinyGPS++.h"

class GNSS {
public:
    GNSS(Stream &gnssStream, uint8_t powerPin);

    void begin(uint32_t baud = 115200);
    void update();

    bool hasFix();

    TinyGPSPlus &getGPS();
    double latitude();
    double longitude();
    uint32_t satellites();
    uint8_t day();
    uint8_t month();
    uint16_t year();
    uint8_t hour();
    uint8_t minute();
    uint8_t second();

private:
    TinyGPSPlus gps;
    Stream &serial;
    uint8_t vgnssCtrlPin;
};
