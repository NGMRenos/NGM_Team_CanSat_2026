#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "GNSS.hpp"
#include "monitor.hpp"
#include "Lora.hpp"
#include "SDCard.hpp"
#include "BME280.hpp"
#include "GamepadDrive.hpp"

// --- Pins ---
#define SPI_SCK  9
#define SPI_MISO 11
#define SPI_MOSI 10

#define VGNSS_CTRL 3
#define GNSS_RX  33
#define GNSS_TX  34

// I2C Pins Heltec Wireless Tracker
#define I2C_SDA 45
#define I2C_SCL 46



HardwareSerial GNSSSerial(1);
GNSS gnss(GNSSSerial, VGNSS_CTRL);
Monitor monitor;
SDCard sdCard;
BME280Sensor bme;     // ★ BME280 Sensor
GamepadDrive roverDrive;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- SYSTEM BOOT ---");

    // GNSS INIT
    GNSSSerial.setRxBufferSize(1024);
    GNSSSerial.begin(115200, SERIAL_8N1, GNSS_RX, GNSS_TX);
    gnss.begin();

    // SPI BUS INIT
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    delay(100);

    // I2C SCANNER & INIT
    Serial.println("I2C Scanner starting...");
    Wire.begin(I2C_SDA, I2C_SCL); 
    Wire.setClock(400000);
    
    // int nDevices = 0;
    // for (uint8_t address = 1; address < 127; address++) {
    //     Wire.beginTransmission(address);
    //     if (Wire.endTransmission() == 0) {
    //         Serial.printf("Found I2C device at: 0x%02X\n", address);
    //         nDevices++;
    //     }
    // }
    // if (nDevices == 0) Serial.println("No I2C devices found.");
    // Serial.println("Scanner done.");

    // SD INIT
    Serial.print("SD Init... ");
    if (sdCard.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAIL");
    }

    // LORA INIT
    Serial.print("LoRa Init... ");
    if (LoRa.begin(SPI)) {
        Serial.println("OK");
    } else {
        Serial.println("FAIL");
    }

    // ★ BME280 INIT
    Serial.print("BME280... ");
    if (bme.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAIL");
    }

    monitor.begin(115200);
    roverDrive.begin(); 
    Serial.println("System Ready.");
}

void loop() {
    roverDrive.update();
    // 1. GNSS Update
    gnss.update();

    uint8_t h = gnss.hour();
    uint8_t m = gnss.minute();
    uint8_t s = gnss.second();
    uint8_t d = gnss.day();
    uint8_t mo = gnss.month();
    uint16_t y = gnss.year();
    double lat = gnss.latitude();
    double lon = gnss.longitude();
    uint32_t sats = gnss.satellites();

    monitor.updateGNSS(h, m, s, lat, lon, sats);

    // 2. BME280 Update
    bme.update();
    if (bme.isAvailable()) {
        monitor.updateBME(bme.temperature(), bme.pressure(), bme.humidity());
    }

    // 4. Transmit, Logging & Serial Print (Εκτελείται κάθε 750ms)
    static uint32_t lastTx = 0;
    if (millis() - lastTx > 750) {
        lastTx = millis();

        float temp = bme.temperature();
        float press = bme.pressure();

        // ★ ΜΕΤΑΦΕΡΑΜΕ ΤΑ PRINT ΕΔΩ ΜΕΣΑ!
        // Πλέον θα τυπώνονται σωστά, 1 φορά κάθε 750 milliseconds
        if (bme.isAvailable()) {
            Serial.printf("BME T=%.1f°C H=%.1f%% P=%.0fhPa | ", 
                          temp, bme.humidity(), press);
        }
        Serial.printf("GPS %02d:%02d:%02d Sats: %d\n", h, m, s, sats);

        // LoRa αποστολή
        LoRa.sendCombined(lat, lon, sats, h, m, s, temp, press);

        // SD αποθήκευση
        if (sdCard.isAvailable()) {
            sdCard.logMessage(lat, lon, sats, h, m, s, d, mo, y, temp, press);
        }

    lastTx = millis();
}

    
    // delay(20);
}
