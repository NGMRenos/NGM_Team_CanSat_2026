#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "GNSS.hpp"
#include "monitor.hpp"
#include "Lora.hpp"
#include "SDCard.hpp"
#include "BME280.hpp"
#include "ENS160.hpp"
#include "UVSensor.hpp"
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

#define UVSENSOR_PIN 18

HardwareSerial GNSSSerial(1);
GNSS gnss(GNSSSerial, VGNSS_CTRL);
Monitor monitor;
SDCard sdCard;
BME280Sensor bme;     // ★ BME280 Sensor
ENS160Sensor ens160(0x52); // ENS160 @ 0x52
UVSensor uv(UVSENSOR_PIN); 
GamepadDrive roverDrive;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- SYSTEM BOOT ---");

    // GNSS INIT
    GNSSSerial.begin(115200, SERIAL_8N1, GNSS_RX, GNSS_TX);
    gnss.begin();

    // SPI BUS INIT
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    delay(100);

    // I2C SCANNER & INIT
    Serial.println("I2C Scanner starting...");
    Wire.begin(I2C_SDA, I2C_SCL); 
    
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

    // ENS160 INIT
    Serial.print("ENS160... ");
    if (ens160.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAIL");
    }

    uv.begin();
    Serial.println("UV sensor init done");

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
        // ★ Περνάμε Temp, Pressure ΚΑΙ Humidity
        monitor.updateBME(bme.temperature(), bme.pressure(), bme.humidity());
    }

    // 3. ENS160 Update & Serial Display
    if (ens160.isAvailable()) {
    static bool warmedUp = false;
    if (millis() < 60000UL && !warmedUp) {
        Serial.println("ENS160 warming up...");
        monitor.updateENS(0, 0, 0);
    } else {
        warmedUp = true;
        
        // ★ Real-time compensation (άριστο!)
        ens160.setEnvData(bme.temperature(), bme.humidity()); 
        ens160.update();

        uint8_t aqi = ens160.getAQI();
        uint16_t tvoc = ens160.getTVOC();
        uint16_t eco2 = ens160.geteCO2();
        
        // ★ Simple smoothing
        static uint16_t tvoc_avg = tvoc, eco2_avg = eco2;
        tvoc_avg = tvoc_avg * 0.8 + tvoc * 0.2;  // EMA
        eco2_avg = eco2_avg * 0.8 + eco2 * 0.2;
        
        monitor.updateENS(aqi, (uint16_t)tvoc_avg, (uint16_t)eco2_avg);
        Serial.printf("ENS AQI=%d TVOC=%d(avg) eCO2=%d(avg)ppm | ", 
                      aqi, (uint16_t)tvoc_avg, (uint16_t)eco2_avg);
        }
    }

    // Serial Print BME280
    if (bme.isAvailable()) {
        Serial.printf("BME T=%.1f°C H=%.1f%% P=%.0fhPa | ", 
                      bme.temperature(), bme.humidity(), bme.pressure());
    }

    Serial.printf("GPS %02d:%02d:%02d Sats: %d\n", h, m, s, sats);

    uv.update();
    uint16_t uvInt = (uint16_t)(uv.intensity() + 0.5f);  // Round σε uint16_t
    Serial.printf("UV raw=%d, V=%.3f, I=%.1f mW/cm2\n",
        uv.raw(), uv.voltage(), uv.intensity());

    // 4. Transmit & Logging
    static uint32_t lastTx = 0;
if (millis() - lastTx > 750) {

    float temp = bme.temperature();
    float press = bme.pressure();
    uint8_t aqi = ens160.getAQI();
    uint16_t tvoc = ens160.getTVOC();
    uint16_t eco2 = ens160.geteCO2();

    // LoRa με ENS160
    LoRa.sendCombined(lat, lon, sats, h, m, s,
                      temp, press,
                      aqi, tvoc, eco2, uvInt);

    // SD με ENS160
    if (sdCard.isAvailable()) {
        sdCard.logMessage(lat, lon, sats, h, m, s, d, mo, y,
                          temp, press,
                          aqi, tvoc, eco2, uvInt);
    }

    lastTx = millis();
}

    
    delay(20);
}
