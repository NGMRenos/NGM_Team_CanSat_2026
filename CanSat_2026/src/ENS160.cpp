#include "ENS160.hpp"

// ENS160 Registers
#define REG_PART_ID 0x00
#define REG_OPMODE  0x10
#define REG_CONFIG  0x11
#define REG_COMMAND 0x12
#define REG_TEMP_IN 0x13
#define REG_RH_IN   0x15
#define REG_DATA_STATUS 0x20
#define REG_DATA_AQI    0x21
#define REG_DATA_TVOC   0x22
#define REG_DATA_ECO2   0x24
#define REG_DATA_ETOH   0x22 // Same as TVOC usually in diff mode
#define REG_DATA_T      0x30 
#define REG_DATA_RH     0x32

// OPMODES
#define OPMODE_DEEP_SLEEP 0x00
#define OPMODE_IDLE       0x01
#define OPMODE_STD        0x02

ENS160Sensor::ENS160Sensor(uint8_t addr) : i2cAddress(addr), available(false) {}

bool ENS160Sensor::begin() {
    // ΣΗΜΑΝΤΙΚΟ: Δεν καλούμε Wire.begin() εδώ. 
    // Το Wire.begin(45, 46) γίνεται ΜΙΑ φορά στο main.cpp setup()
    
    // Έλεγχος Part ID
    Wire.beginTransmission(i2cAddress);
    Wire.write(REG_PART_ID);
    if (Wire.endTransmission() != 0) return false;

    Wire.requestFrom(i2cAddress, (uint8_t)2);
    if (Wire.available() < 2) return false;
    
    uint16_t partId = Wire.read();
    partId |= (uint16_t)Wire.read() << 8;

    if (partId == 0x0160) { // ENS160 ID
        available = true;
        
        // Set to Standard Mode
        Wire.beginTransmission(i2cAddress);
        Wire.write(REG_OPMODE);
        Wire.write(OPMODE_STD);
        Wire.endTransmission();
        delay(50); // Wait for mode switch
        return true;
    }
    
    return false;
}

bool ENS160Sensor::isAvailable() {
    return available;
}

void ENS160Sensor::setEnvData(float tempC, float humidity) {
    if (!available) return;
    
    // Formula from datasheet: T = (tempC + 273.15) * 64
    uint16_t t_data = (uint16_t)((tempC + 273.15) * 64);
    
    // Formula: RH = humidity * 512
    uint16_t h_data = (uint16_t)(humidity * 512);

    Wire.beginTransmission(i2cAddress);
    Wire.write(REG_TEMP_IN);
    Wire.write(t_data & 0xFF);
    Wire.write((t_data >> 8) & 0xFF);
    Wire.write(h_data & 0xFF);
    Wire.write((h_data >> 8) & 0xFF);
    Wire.endTransmission();
}

void ENS160Sensor::update() {
    // No specific update needed for ENS160 standard mode, 
    // data is read directly in getters.
}

uint8_t ENS160Sensor::getAQI() {
    if (!available) return 0;
    Wire.beginTransmission(i2cAddress);
    Wire.write(REG_DATA_AQI);
    Wire.endTransmission();
    
    Wire.requestFrom(i2cAddress, (uint8_t)1);
    if (Wire.available()) return Wire.read();
    return 0;
}

uint16_t ENS160Sensor::getTVOC() {
    if (!available) return 0;
    Wire.beginTransmission(i2cAddress);
    Wire.write(REG_DATA_TVOC);
    Wire.endTransmission();
    
    Wire.requestFrom(i2cAddress, (uint8_t)2);
    if (Wire.available() >= 2) {
        uint16_t val = Wire.read();
        val |= (uint16_t)Wire.read() << 8;
        return val;
    }
    return 0;
}

uint16_t ENS160Sensor::geteCO2() {
    if (!available) return 0;
    Wire.beginTransmission(i2cAddress);
    Wire.write(REG_DATA_ECO2);
    Wire.endTransmission();
    
    Wire.requestFrom(i2cAddress, (uint8_t)2);
    if (Wire.available() >= 2) {
        uint16_t val = Wire.read();
        val |= (uint16_t)Wire.read() << 8;
        return val;
    }
    return 0;
}

// Dummy implementations for extra getters if needed
uint16_t ENS160Sensor::getHP0() { return 0; }
uint16_t ENS160Sensor::getHP1() { return 0; }
uint16_t ENS160Sensor::getHP2() { return 0; }
uint16_t ENS160Sensor::getHP3() { return 0; }
