#pragma once
#include <Arduino.h>
#include <RadioLib.h>

// Αφαίρεσα το extern - η LoRa είναι global instance

class LoRaClass {
public:
    LoRaClass();
    bool begin(SPIClass& spi);
    bool sendGNSS(double lat, double lon, uint32_t sats, uint8_t h, uint8_t m, uint8_t s);
    bool sendCombined(double lat, double lon, uint32_t sats,
                  uint8_t h, uint8_t m, uint8_t s,
                  float temp, float pressure);

private:
    Module* module = nullptr;
    SX1262* radio = nullptr;
    
    uint8_t createPacket(double lat, double lon, uint32_t sats, uint8_t h, uint8_t m, uint8_t s, uint8_t* out, size_t outMax) const;
    uint8_t createCombinedPacket(double lat, double lon, uint32_t sats,
                             uint8_t h, uint8_t m, uint8_t s,
                             float temp, float pressure,
                             uint8_t *out, size_t outMax) const;
    #define RADIO_NSS  8
    #define RADIO_DIO1 14
    #define RADIO_RESET 12
    #define RADIO_BUSY 13
    #define MY_NETWORK_ID 0x55
};

// Global instance
extern LoRaClass LoRa;
