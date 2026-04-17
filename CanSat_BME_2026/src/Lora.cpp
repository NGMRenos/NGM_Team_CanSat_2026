#include "Lora.hpp"

LoRaClass LoRa;  // Global definition εδώ!

LoRaClass::LoRaClass() {}

bool LoRaClass::begin(SPIClass& spi) {
    pinMode(RADIO_NSS, OUTPUT);
    
    module = new Module(RADIO_NSS, RADIO_DIO1, RADIO_RESET, RADIO_BUSY, spi);
    radio = new SX1262(module);
    
    int16_t state = radio->begin(869.5, 250.0, 8, 5, 0x12, 22, 8, 1.8, false);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("SX1262 Init Failed: %d\n", state);
        return false;
    }
    
    radio->setDio2AsRfSwitch(true);
    Serial.println("LoRa SX1262 Init OK!");
    return true;
}

bool LoRaClass::sendGNSS(double lat, double lon, uint32_t sats, uint8_t h, uint8_t m, uint8_t s) {
    if (!radio) return false;
    
    uint8_t packet[16];
    uint8_t len = createPacket(lat, lon, sats, h, m, s, packet, sizeof(packet));
    if (len == 0) return false;
    
    int16_t state = radio->transmit(packet, len);
    Serial.printf("TX GNSS: %s\n", state == RADIOLIB_ERR_NONE ? "OK" : "FAIL");
    return state == RADIOLIB_ERR_NONE;
}

bool LoRaClass::sendCombined(double lat, double lon, uint32_t sats,
                             uint8_t h, uint8_t m, uint8_t s,
                             float temp, float pressure) {
    if (!radio) return false;

    uint8_t packet[35];
    uint8_t len = createCombinedPacket(lat, lon, sats, h, m, s,
                                       temp, pressure,
                                       packet, sizeof(packet));
    if (len == 0) return false;

    int16_t state = radio->transmit(packet, len);
    Serial.printf("TX Combined: %s\n", state == RADIOLIB_ERR_NONE ? "OK" : "FAIL");
    return state == RADIOLIB_ERR_NONE;
}

uint8_t LoRaClass::createPacket(double lat, double lon, uint32_t sats, uint8_t h, uint8_t m, uint8_t s, uint8_t* out, size_t outMax) const {
    if (outMax < 16) return 0;
    
    out[0] = 0x47;  // G GNSS
    out[1] = MY_NETWORK_ID;
    
    int32_t late7 = (int32_t)(lat * 1e7);
    memcpy(&out[2], &late7, 4);
    
    int32_t lone7 = (int32_t)(lon * 1e7);
    memcpy(&out[6], &lone7, 4);
    
    uint16_t s16 = (sats > 65535) ? 65535 : (uint16_t)sats;
    out[10] = (s16 >> 8) & 0xFF;
    out[11] = s16 & 0xFF;
    
    out[12] = ((h / 10) << 4) | (h % 10);
    out[13] = ((m / 10) << 4) | (m % 10);
    out[14] = ((s / 10) << 4) | (s % 10);
    
    uint8_t chk = 0;
    for (int i = 0; i < 15; i++) chk ^= out[i];
    out[15] = chk;
    
    return 16;
}

uint8_t LoRaClass::createCombinedPacket(double lat, double lon, uint32_t sats, uint8_t h, uint8_t m, uint8_t s,
                             float temp, float pressure, uint8_t* out, size_t outMax) const {
    if (outMax < 26) return 0;

    out[0] = 0x43; // 'C' Combined
    out[1] = MY_NETWORK_ID;

    // GNSS part
    int32_t lat_e7 = (int32_t)(lat * 1e7);
    memcpy(&out[2], &lat_e7, 4);
    int32_t lon_e7 = (int32_t)(lon * 1e7);
    memcpy(&out[6], &lon_e7, 4);

    uint16_t s16 = (sats > 65535) ? 65535 : (uint16_t)sats;
    out[10] = (s16 >> 8) & 0xFF;
    out[11] = s16 & 0xFF;

    // ★ ★ ★ ΔΙΟΡΘΩΜΕΝΟ: BCD time όπως GNSS packet!
    out[12] = ((h / 10) << 4) | (h % 10);
    out[13] = ((m / 10) << 4) | (m % 10);
    out[14] = ((s / 10) << 4) | (s % 10);

    // BME part
    int16_t t10 = (int16_t)(temp * 10);
    memcpy(&out[15], &t10, 2);
    uint16_t p10 = (uint16_t)(pressure * 10);
    memcpy(&out[17], &p10, 2);

  
    uint8_t chk = 0;
    for (int i = 0; i < 26; i++) chk ^= out[i];
    out[26] = chk;
    return 27;
}
