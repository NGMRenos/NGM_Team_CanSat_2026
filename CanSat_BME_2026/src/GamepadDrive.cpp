#include "GamepadDrive.hpp"
#include <NimBLEDevice.h>

// ----- GPIO (Ακριβώς όπως τα ζήτησες) -----
const int AIN1_PIN = 15;   
const int AIN2_PIN = 16;   
const int BIN1_PIN = 37;   // ΠΡΟΣΟΧΗ: Αυτό είναι και το Vext_Ctrl (Ρεύμα Servos)
const int BIN2_PIN = 26;   
const int EEP_PIN  = 3;    
const int SERVO_PIN = 17;  

// ----- PWM Settings -----
const int PWM_FREQ = 20000;
const int PWM_RES = 8;    
const int CH_A_FWD = 0;
const int CH_A_REV = 1;
const int CH_B_FWD = 2;
const int CH_B_REV = 3;

const int CH_SERVO = 4;
const int PWM_FREQ_SERVO = 50; 
const int PWM_RES_SERVO = 12;  

// ----- Bluetooth Settings -----
static NimBLEAddress serverAddress("27:81:b0:a4:0e:ce"); 
static const NimBLEUUID serviceUUID("1812");             
static const NimBLEUUID charUUID("2a4d");                

#define DEADZONE 2000
static volatile uint8_t rawData[16];
static volatile bool newDataAvailable = false;
static bool connected = false;
static NimBLEClient* pClient = nullptr;

// ----- Συναρτήσεις Κίνησης -----

static void setServoAngle(int angle) {
    angle = constrain(angle, 0, 180);
    int duty = map(angle, 0, 180, 111, 491);
    ledcWrite(CH_SERVO, duty);
}

static void setMotor(int speed, int chFwd, int chRev) {
    speed = constrain(speed, -255, 255);
    

    if (speed == 0) {
        ledcWrite(chFwd, 0);
        ledcWrite(chRev, 0);
        return;
    }

    if (speed > 0) {
        ledcWrite(chFwd, speed);
        ledcWrite(chRev, 0);
    } else {
        ledcWrite(chFwd, 0);
        ledcWrite(chRev, -speed);
    }
}

static void stopMotors() {
    setMotor(0, CH_A_FWD, CH_A_REV);
    setMotor(0, CH_B_FWD, CH_B_REV);
}

// ----- BLE CALLBACKS -----

static int16_t mapJoystick(uint8_t low, uint8_t high) {
    uint16_t raw = low | (high << 8);
    long value = (long)raw - 32768;
    if (abs(value) < DEADZONE) value = 0;
    return (int16_t)value;
}

static void notifyCallback(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
    if (length >= 14) { 
        memcpy((void*)rawData, pData, min(length, (size_t)16));
        newDataAvailable = true;
    }
}

class MyClientCallback : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) override { Serial.println("Gamepad: Connected!"); }
    void onDisconnect(NimBLEClient* pClient) override { stopMotors(); connected = false; }
};

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
    pClient->secureConnection();
    NimBLERemoteService* pSvc = pClient->getService(serviceUUID);
    if (pSvc) {
        std::vector<NimBLERemoteCharacteristic*>* pChars = pSvc->getCharacteristics(&charUUID);
        if (pChars) {
            for (auto pChar : *pChars) { if (pChar->canNotify()) pChar->subscribe(true, notifyCallback); }
            connected = true; return true;
        }
    }
    return false;
}

void GamepadDrive::begin() {
    // 0. ΕΝΕΡΓΟΠΟΙΗΣΗ DRV8833 (ΞΥΠΝΗΜΑ)
    // Στην v1.1 το GPIO 3 είναι το Vext Ctrl, το κρατάμε HIGH για τον driver
    pinMode(EEP_PIN, OUTPUT);
    digitalWrite(EEP_PIN, HIGH); 

    // 1. SETUP ΜΟΤΕΡ
    pinMode(AIN1_PIN, OUTPUT);
    pinMode(AIN2_PIN, OUTPUT);
    pinMode(BIN1_PIN, OUTPUT);
    pinMode(BIN2_PIN, OUTPUT);

    ledcSetup(CH_A_FWD, PWM_FREQ, PWM_RES);
    ledcSetup(CH_A_REV, PWM_FREQ, PWM_RES);
    ledcSetup(CH_B_FWD, PWM_FREQ, PWM_RES);
    ledcSetup(CH_B_REV, PWM_FREQ, PWM_RES);

    ledcAttachPin(AIN1_PIN, CH_A_FWD);
    ledcAttachPin(AIN2_PIN, CH_A_REV);
    ledcAttachPin(BIN1_PIN, CH_B_FWD);
    ledcAttachPin(BIN2_PIN, CH_B_REV);
    
    // Σταματάμε τα μοτέρ αμέσως (χωρίς κίνηση)
    stopMotors();

    // 2. SETUP SERVO (12-bit)
    ledcSetup(CH_SERVO, PWM_FREQ_SERVO, PWM_RES_SERVO);
    ledcAttachPin(SERVO_PIN, CH_SERVO);
    
    // Θέτουμε το servo στην αρχική θέση (π.χ. 90 μοίρες) 
    // χωρίς delay, οπότε δεν θα υπάρξει αναμονή ή τεστ.
    setServoAngle(90); 

    // 3. BLE SETUP
    NimBLEDevice::init("");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    
    Serial.println("Gamepad Drive: Init OK (No test move).");
}

void GamepadDrive::update() {
    if (!connected) {
        static uint32_t lastScan = 0;
        if (millis() - lastScan > 3000) { lastScan = millis(); connectToGameSir(); }
    } else {
        if (newDataAvailable) {
            // --- ΕΛΕΓΧΟΣ ΚΟΥΜΠΙΩΝ (Έλεγχος σε Byte 11 ΚΑΙ 13 για σιγουριά) ---
            bool lb = (rawData[13] & 0x40) || (rawData[11] & 0x01);
            bool rb = (rawData[13] & 0x80) || (rawData[11] & 0x02);

            if (lb) {
                setServoAngle(0);
                Serial.println("Servo -> 0");
            } else if (rb) {
                setServoAngle(90);
                Serial.println("Servo -> 90");
            }

            // --- TANK DRIVE ---
            int16_t joyLY = mapJoystick(rawData[2], rawData[3]);
            int16_t joyRY = mapJoystick(rawData[6], rawData[7]);

            setMotor((int)(-(float)joyLY / 32768.0f * 255), CH_A_FWD, CH_A_REV);
            setMotor((int)(-(float)joyRY / 32768.0f * 255), CH_B_FWD, CH_B_REV);

            newDataAvailable = false;
        }
    }
}