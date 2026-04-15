#include "SDCard.hpp"

#define SD_CS   7
#define SD_MOSI 5
#define SD_MISO 4
#define SD_SCK  6

SDCard::SDCard() {}

bool SDCard::begin() {
    _spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, _spi)) {
        Serial.println("SD mount fail");
        return false;
    }
    
    _initialized = true;
    
    if (!SD.exists(_filename)) {
        File f = SD.open(_filename, FILE_WRITE);
        if (f) {
            f.println("Timestamp,DateTime,Lat,Lon,Sats,TempC,PhPa,AQI,TVOCppb,eCO2ppm,UVmWcm2");
            f.close();
            Serial.println("Created loralog.csv");
        }
    }
    Serial.println("SD OK");
    return true;
}

bool SDCard::logMessage(double lat, double lon, uint32_t sats,
                        uint8_t h, uint8_t m, uint8_t s,
                        uint8_t day, uint8_t month, uint16_t year,
                        float temp, float press,
                        uint8_t aqi, uint16_t tvoc, uint16_t eco2, uint16_t uv) {
    if (!_initialized) return false;

    char dataBuffer[260];
    snprintf(dataBuffer, sizeof(dataBuffer),
             "%lu,%02d/%02d/%04d %02d:%02d:%02d,%.6f,%.6f,%lu,%.1f,%.1f,%u,%u,%u",
             millis(),
             day, month, year,
             h, m, s,
             lat, lon,
             (unsigned long)sats,
             temp, press,
             aqi, (unsigned int)tvoc, (unsigned int)eco2, (unsigned int)uv);

    File f = SD.open(_filename, FILE_APPEND);
    if (!f) return false;
    f.println(dataBuffer);
    f.close();

    Serial.print("SD: ");
    Serial.println(dataBuffer);
    return true;
}


bool SDCard::isAvailable() { return _initialized; }
