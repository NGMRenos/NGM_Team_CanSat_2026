#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

// Pins TTGO v1
#define LORA_NSS  18
#define LORA_RST  14
#define LORA_DIO0 26
#define LORA_DIO1 33
#define LORA_SCK  5
#define LORA_MISO 19
#define LORA_MOSI 27

#define LORA_FREQ 869.5
#define LORA_BW 250.0
#define LORA_SF 8
#define LORA_CR 5
#define LORA_SW 0x12
#define LORA_POWER 20
#define MY_NETWORK_ID 0x55

Module mod(LORA_NSS, LORA_DIO0, LORA_RST, RADIOLIB_NC);
SX1276 radio(&mod);

QueueHandle_t packetQueue;
volatile bool got = false;

#if defined(ESP32)
  void IRAM_ATTR setFlag() { got = true; }
#else
  void setFlag() { got = true; }
#endif

struct CombinedPacket {
  uint8_t header;           
  uint8_t net_id;
  int32_t lat_e7, lon_e7;
  uint16_t satellites;   
  uint8_t time_bytes[3];  // ★ Μετονομασία: decimal bytes
  int16_t temp_x10;         
  uint16_t pressure_x10;    
  uint8_t aqi;
  uint16_t tvoc_ppb;
  uint16_t eco2_ppm;
  uint16_t uv_mWcm2_x10;
  uint8_t checksum;
  int16_t rssi;
  float snr;
  uint32_t timestamp;
};

void TaskLoRa(void *pvParameters) {
  for (;;) {
    if (got) {
      got = false;
      uint8_t buf[256];
      int st = radio.readData(buf, sizeof(buf));
      
      if (st == RADIOLIB_ERR_NONE) {
        int len = radio.getPacketLength();
        
        if ((len == 16 || len == 20 || len == 25 || len == 27) && buf[1] == MY_NETWORK_ID) {
          
          CombinedPacket pkt = {0};
          pkt.header = buf[0];
          pkt.net_id = buf[1];
          
          memcpy(&pkt.lat_e7, &buf[2], 4);
          memcpy(&pkt.lon_e7, &buf[6], 4);
          pkt.satellites = ((uint16_t)buf[10] << 8) | buf[11];
          memcpy(pkt.time_bytes, &buf[12], 3);  // ★ time_bytes
            
          if (len >= 20) {
            memcpy(&pkt.temp_x10, &buf[15], 2);
            memcpy(&pkt.pressure_x10, &buf[17], 2);
          } else {
            pkt.temp_x10 = -999;
            pkt.pressure_x10 = -999;
          }

          if (len >= 25) {
            pkt.aqi = buf[19];
            pkt.tvoc_ppb = ((uint16_t)buf[20] << 8) | buf[21];
            pkt.eco2_ppm = ((uint16_t)buf[22] << 8) | buf[23];
            
            if (len == 27) {
              pkt.uv_mWcm2_x10 = ((uint16_t)buf[24] << 8) | buf[25];
              pkt.checksum = buf[26];
            } else {
              pkt.uv_mWcm2_x10 = 0;
              pkt.checksum = buf[len-1];
            }
          } else {
            pkt.aqi = 0; pkt.tvoc_ppb = 0; pkt.eco2_ppm = 0; pkt.uv_mWcm2_x10 = 0;
            pkt.checksum = buf[len-1];
          }
          
          pkt.rssi = radio.getRSSI();
          pkt.snr = radio.getSNR();
          pkt.timestamp = millis();

          xQueueSend(packetQueue, &pkt, 0);
        }
      }
      radio.startReceive();
    }
    vTaskDelay(5 / portTICK_PERIOD_MS); 
  }
}

void printCombinedData(const CombinedPacket& pkt) {
  double lat = pkt.lat_e7 / 1e7;
  double lon = pkt.lon_e7 / 1e7;
  
  // ★ ΔΙΑΒΑΣΜΑ ΩΣ DECIMAL (όπως στέλνει TinyGPS++)
  uint8_t hh = pkt.time_bytes[0];  // 0x0F = 15
  uint8_t mm = pkt.time_bytes[1];  // 0x2D = 45
  uint8_t ss = pkt.time_bytes[2];  // 0x02 = 2
  
  static uint32_t prev = 0;
  uint32_t diff = pkt.timestamp - prev;
  prev = pkt.timestamp;

  Serial.println("================================");
  Serial.printf("RX | Type:%c | Net:0x%02X | Δt:%lums\n", 
                pkt.header, pkt.net_id, diff);
  Serial.printf("🕐 %02d:%02d:%02d (GNSS time)\n", hh, mm, ss);  // ★ ΣΩΣΤΗ ΩΡΑ!
  
  Serial.printf("📍 %.6f°N %.6f°E\n", lat, lon);
  Serial.printf("🛰️  Sats: %u\n", pkt.satellites);
  
  if(pkt.temp_x10 != -999) {
    Serial.printf("🌡️  TEMP: %.1f°C\n", pkt.temp_x10/10.0f);
    Serial.printf("🌪️  PRESS: %.0f hPa\n", pkt.pressure_x10/10.0f);
  }
  
  if (pkt.aqi > 0) {
    Serial.printf("🌬️  AQI: %d\n", pkt.aqi);
    Serial.printf("💨  TVOC: %u ppb\n", pkt.tvoc_ppb);
    Serial.printf("🥫  eCO2: %u ppm\n", pkt.eco2_ppm);
  }
  
  if (pkt.uv_mWcm2_x10 > 0) {
    Serial.printf("☀️  UV: %.1f mW/cm²\n", pkt.uv_mWcm2_x10 / 10.0f);
  }
  
  Serial.printf("📶 RSSI:%d dBm SNR:%.1f dB\n", pkt.rssi, pkt.snr);
  Serial.printf("⏱️  %lu ms\n", pkt.timestamp);
  Serial.println("================================");
}

void setup() {
  Serial.begin(115200);
  
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  delay(10);
  
  pinMode(LORA_RST, OUTPUT);
  digitalWrite(LORA_RST, LOW); delay(10);
  digitalWrite(LORA_RST, HIGH); delay(10);

  Serial.print("SX1276... ");
  int16_t st = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, LORA_SW, LORA_POWER);
  
  if (st == RADIOLIB_ERR_NONE) {
    Serial.println("OK!");
    radio.setCRC(true);
    radio.setDio0Action(setFlag, RISING);
    radio.startReceive();
    
    packetQueue = xQueueCreate(20, sizeof(CombinedPacket));
    xTaskCreatePinnedToCore(TaskLoRa, "LoRa", 10000, NULL, 1, NULL, 0);
    
    Serial.println("🔥 GNSS+BME+ENS+UV Receiver Ready!");
    
  } else {
    Serial.printf("❌ FAIL %d\n", st);
    while(1);
  }
}

void loop() {
  CombinedPacket pkt;
  if (xQueueReceive(packetQueue, &pkt, portMAX_DELAY) == pdTRUE) {
    printCombinedData(pkt);
  }
}
