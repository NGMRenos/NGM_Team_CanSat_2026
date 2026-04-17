#pragma once

#include <Arduino.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>

class Monitor {
public:
    Monitor();
    void begin(uint32_t serialBaud = 115200);

    void updateGNSS(uint8_t hour, uint8_t minute, uint8_t second, 
                    double latitude, double longitude, uint32_t satellites);
    
    // ★ Προσθήκη humidity (συμβατό με BME280)
    void updateBME(float temp, float pressure, float humidity); 


    Adafruit_ST7735& getDisplay() { return display; }

private:
    Adafruit_ST7735 display;
    uint32_t lastUpdate;
    const uint32_t updateIntervalMs = 2000;

    void printSerial(uint8_t hour, uint8_t minute, uint8_t second, 
                     double latitude, double longitude, uint32_t satellites);
                     
    void printDisplay(uint8_t hour, uint8_t minute, uint8_t second, 
                      double latitude, double longitude, uint32_t satellites);
    
    // Εσωτερικές μέθοδοι εμφάνισης
    void printBMEDisplay(float temp, float pressure, float humidity);

};
