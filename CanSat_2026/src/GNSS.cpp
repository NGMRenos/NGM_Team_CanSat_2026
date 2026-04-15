#include "GNSS.hpp"

GNSS::GNSS(Stream &gnssStream, uint8_t powerPin)
  : serial(gnssStream), vgnssCtrlPin(powerPin) {}

void GNSS::begin(uint32_t baud) {
  pinMode(vgnssCtrlPin, OUTPUT);
  digitalWrite(vgnssCtrlPin, HIGH);
}

void GNSS::update() {
  while (serial.available() > 0) {
    gps.encode(serial.read());
  }
}

bool GNSS::hasFix() {
  return gps.location.isValid() && gps.location.isUpdated();
}

TinyGPSPlus &GNSS::getGPS() {
  return gps;
}

double GNSS::latitude() {
  return gps.location.isValid() ? gps.location.lat() : 0.0;
}

double GNSS::longitude() {
  return gps.location.isValid() ? gps.location.lng() : 0.0;
}

uint32_t GNSS::satellites() {
  return gps.satellites.isValid() ? gps.satellites.value() : 0;
}

uint8_t GNSS::day() {
  return gps.date.day();
}

uint8_t GNSS::month() {
  return gps.date.month();
}

uint16_t GNSS::year() {
  return gps.date.year();
}

uint8_t GNSS::hour() {
  return gps.time.isValid() ? gps.time.hour() : 0;
}

uint8_t GNSS::minute() {
  return gps.time.isValid() ? gps.time.minute() : 0;
}

uint8_t GNSS::second() {
  return gps.time.isValid() ? gps.time.second() : 0;
}
