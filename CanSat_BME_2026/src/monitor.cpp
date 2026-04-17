#include "monitor.hpp"

// TFT Pins
#define TFT_CS    38
#define TFT_RST   39
#define TFT_DC    40
#define TFT_MOSI  42
#define TFT_SCLK  41
#define TFT_BL    21

Monitor::Monitor() 
    : display(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST), lastUpdate(0) {}

void Monitor::begin(uint32_t serialBaud) {
    Serial.begin(serialBaud);
    // Init TFT 160x80
    display.initR(INITR_GREENTAB);
    display.setRotation(1);
    display.invertDisplay(true);
    display.fillScreen(ST7735_BLACK);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    Serial.println("TFT Display initialized");
}

void Monitor::updateGNSS(uint8_t hour, uint8_t minute, uint8_t second, 
                         double latitude, double longitude, uint32_t satellites) {
    uint32_t now = millis();
    if (now - lastUpdate < updateIntervalMs) return;
    lastUpdate = now;

    printSerial(hour, minute, second, latitude, longitude, satellites);
    printDisplay(hour, minute, second, latitude, longitude, satellites);
}

void Monitor::updateBME(float temp, float pressure, float humidity) {
    printBMEDisplay(temp, pressure, humidity);
}


void Monitor::printSerial(uint8_t hour, uint8_t minute, uint8_t second, 
                          double latitude, double longitude, uint32_t satellites) {
    Serial.printf("Time %02d:%02d:%02d Lat %.6f Lon %.6f Sats %lu\n", 
                  hour, minute, second, latitude, longitude, (unsigned long)satellites);
}

void Monitor::printDisplay(uint8_t hour, uint8_t minute, uint8_t second, 
                           double latitude, double longitude, uint32_t satellites) {
    // Καθαρισμός μόνο του πάνω μέρους (GPS data)
    display.fillRect(0, 0, 160, 60, ST7735_BLACK);

    display.setTextColor(ST7735_CYAN, ST7735_BLACK);
    display.setTextSize(1);
    
    char buf[32];
    sprintf(buf, "TIME %02d:%02d:%02d", hour, minute, second);
    display.setCursor(2, 25); // Ήταν 25 στο original, το ανέβασα λίγο για χώρο
    display.print(buf);

    sprintf(buf, "LAT %.6f", latitude);
    display.setCursor(2, 35); // Ήταν 40
    display.print(buf);

    sprintf(buf, "LON %.6f", longitude);
    display.setCursor(2, 45); // Ήταν 55
    display.print(buf);

    display.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    if (satellites > 0) {
        sprintf(buf, "SATS %lu", (unsigned long)satellites);
    } else {
        sprintf(buf, "SATS WAIT...");
    }
    display.setCursor(2, 55); // Ήταν 70
    display.print(buf);
}

void Monitor::printBMEDisplay(float temp, float pressure, float humidity) {
    // Περιοχή κάτω από GPS
    display.setTextColor(ST7735_MAGENTA, ST7735_BLACK);
    display.setTextSize(1);
    
    char buf[32];
    // T και H στην ίδια γραμμή
    sprintf(buf, "T%.1fC H%.0f%%", temp, humidity);
    display.setCursor(2, 65); // Κάτω από SATS
    display.print(buf);

    // P δίπλα ή από κάτω
    sprintf(buf, "P%.0fh", pressure);
    display.setCursor(90, 65); 
    display.print(buf);
}


