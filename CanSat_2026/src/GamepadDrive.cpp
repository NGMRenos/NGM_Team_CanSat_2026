#include "GamepadDrive.hpp"
#include <NimBLEDevice.h>

// ----- GPIO για DRV8833 -----
const int AIN1_PIN = 15;   // Motor A (Left) Forward
const int AIN2_PIN = 48;   // Motor A (Left) Reverse
const int BIN1_PIN = 18;   // Motor B (Right) Forward
const int BIN2_PIN = 47;   // Motor B (Right) Reverse

// ----- GPIO για SERVO -----
const int SERVO_PIN = 16;  

// PWM Settings (Motors)
const int PWM_FREQ = 20000;
const int PWM_RES = 8;    // 0-255
const int CH_A_FWD = 0;
const int CH_A_REV = 1;
const int CH_B_FWD = 2;
const int CH_B_REV = 3;

// PWM Settings (Servo)
const int CH_SERVO = 4;
const int PWM_FREQ_SERVO = 50; // Το Servo δουλεύει αυστηρά στα 50Hz (20ms)
const int PWM_RES_SERVO = 16;  // 16-bit ανάλυση (0-65535)

// ----- Bluetooth Settings -----
static NimBLEAddress serverAddress("27:81:b0:a4:0e:ce");    // GameSir 2
static const NimBLEUUID serviceUUID("1812");   // HID Service
static const NimBLEUUID charUUID("2a4d");      // HID Report

#define DEADZONE 2000

static volatile uint8_t rawData[16];
static volatile bool newDataAvailable = false;
static bool connected = false;
static NimBLEClient* pClient = nullptr;

// ----- Έλεγχος Servo -----
static void setServoAngle(int angle) {
    // Περιορίζουμε τις μοίρες από 0 έως 180
    angle = constrain(angle, 0, 180);
    
    // Στα 50Hz (16-bit = 0-65535), τα τυπικά Servo θέλουν παλμό από 544µs έως 2400µs
    int duty = map(angle, 0, 180, 1782, 7864);
    ledcWrite(CH_SERVO, duty);
}

// ----- Έλεγχος Motors -----
static void setMotor(int speed, int chFwd, int chRev) {
    speed = constrain(speed, -255, 255);
    if (speed > 0) {
        ledcWrite(chFwd, speed);
        ledcWrite(chRev, 0);
    } else if (speed < 0) {
        ledcWrite(chFwd, 0);
        ledcWrite(chRev, -speed);
    } else {
        ledcWrite(chFwd, 0);
        ledcWrite(chRev, 0);
    }
}

static void stopMotors() {
    ledcWrite(CH_A_FWD, 0);
    ledcWrite(CH_A_REV, 0);
    ledcWrite(CH_B_FWD, 0);
    ledcWrite(CH_B_REV, 0);
}

// ----- BLE Callbacks -----
static void notifyCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, 
                           uint8_t* pData, size_t length, bool isNotify) {
    if (length >= 8) {
        memcpy((void*)rawData, pData, min(length, (size_t)16));
        newDataAvailable = true;
    }
}

class MyClientCallback : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) override { 
        Serial.println("BLE Gamepad: Connected!"); 
    }
    
    void onDisconnect(NimBLEClient* pClient) override { 
        Serial.println("BLE Gamepad: Disconnected! Stopping motors...");
        stopMotors();  // Failsafe για τα μοτέρ
        connected = false; 
    }
    
    bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) override { 
        return true; 
    }
};

static int16_t mapJoystick(uint8_t low, uint8_t high) {
    uint16_t raw = low | (high << 8);
    long value = (long)raw - 32768;
    if (abs(value) < DEADZONE) value = 0;
    return (int16_t)value;
}

static bool connectToGameSir() {
    if (NimBLEDevice::getClientListSize()) {
        pClient = NimBLEDevice::getClientByPeerAddress(serverAddress);
        if (pClient) pClient->connect(serverAddress, false);
    }

    if (!pClient || !pClient->isConnected()) {
        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(new MyClientCallback());
        if (!pClient->connect(serverAddress)) return false;
    }

    if (!pClient->secureConnection()) Serial.println("BLE Gamepad: Already bonded.");

    NimBLERemoteService* pSvc = pClient->getService(serviceUUID);
    if (pSvc) {
        std::vector<NimBLERemoteCharacteristic*>* pChars = pSvc->getCharacteristics(&charUUID);
        if (pChars == nullptr) { 
            pClient->disconnect(); 
            return false; 
        }

        int foundCount = 0;
        for (auto pChar : *pChars) {
            if (pChar->canNotify()) {
                pChar->subscribe(true, notifyCallback);
                foundCount++;
            }
        }
        if (foundCount > 0) {
            connected = true;
            return true;
        }
    }
    pClient->disconnect();
    return false;
}

void GamepadDrive::begin() {
    // GPIO setup (Motors)
    pinMode(AIN1_PIN, OUTPUT);
    pinMode(AIN2_PIN, OUTPUT);
    pinMode(BIN1_PIN, OUTPUT);
    pinMode(BIN2_PIN, OUTPUT);

    // PWM setup (Motors)
    ledcSetup(CH_A_FWD, PWM_FREQ, PWM_RES);
    ledcSetup(CH_A_REV, PWM_FREQ, PWM_RES);
    ledcSetup(CH_B_FWD, PWM_FREQ, PWM_RES);
    ledcSetup(CH_B_REV, PWM_FREQ, PWM_RES);

    ledcAttachPin(AIN1_PIN, CH_A_FWD);
    ledcAttachPin(AIN2_PIN, CH_A_REV);
    ledcAttachPin(BIN1_PIN, CH_B_FWD);
    ledcAttachPin(BIN2_PIN, CH_B_REV);

    stopMotors();

    // PWM setup (Servo)
    ledcSetup(CH_SERVO, PWM_FREQ_SERVO, PWM_RES_SERVO);
    ledcAttachPin(SERVO_PIN, CH_SERVO);
    setServoAngle(0); // Αρχική θέση στις 0 μοίρες (μπορείς να το αλλάξεις αν θες)

    // BLE setup
    NimBLEDevice::init("");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    
    Serial.println("BLE Gamepad & Servo Init Done.");
}

void GamepadDrive::update() {
    if (!connected) {
        static uint32_t lastScanTime = 0;
        if (millis() - lastScanTime > 3000) {
            lastScanTime = millis();
            if (!connectToGameSir()) {
                // Serial.println("BLE Gamepad: Scanning...");
            }
        }
    } else {
        if (newDataAvailable) {
            // ----- 1. Έλεγχος Servo με LB / RB -----
            // Το Byte 11 περιέχει συνήθως την κατάσταση των κουμπιών (A, B, X, Y, LB, RB)
            // Bit 4 (0x10) -> LB, Bit 5 (0x20) -> RB
            bool lbPressed = (rawData[11] & 0x10) != 0;
            bool rbPressed = (rawData[11] & 0x20) != 0;

            if (lbPressed) {
                setServoAngle(0);   // Αν πατηθεί το LB, πάει στο 0
            } else if (rbPressed) {
                setServoAngle(90);  // Αν πατηθεί το RB, πάει στο 90
            }

            // Για να δεις στο Serial Monitor όλα τα κουμπιά αν τυχόν στο GameSir σου
            // έχουν διαφορετικό mapping, ξε-σχολίασε την επόμενη γραμμή:
            // Serial.printf("Byte 11: 0x%02X \n", rawData[11]);

            // ----- 2. Έλεγχος Tank Drive (Left Y & Right Y) -----
            int16_t joyLY = mapJoystick(rawData[2], rawData[3]); 
            int16_t joyRY = mapJoystick(rawData[6], rawData[7]); 

            // Normalize (-1.0 ... 1.0)
            float normLeft  = (joyLY == 0) ? 0.0f : -(float)joyLY / 32768.0f;
            float normRight = (joyRY == 0) ? 0.0f : -(float)joyRY / 32768.0f;

            // PWM Conversion
            int motorA = (int)(normLeft * 255);
            int motorB = (int)(normRight * 255);

            setMotor(motorA, CH_A_FWD, CH_A_REV);
            setMotor(motorB, CH_B_FWD, CH_B_REV);

            newDataAvailable = false;
        }
    }
}