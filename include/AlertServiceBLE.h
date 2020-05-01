#include "AlertService.h"

// Library: ESP32 BLE Arduino
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

class AlertServiceBLE : public AlertService {

    public:

        // Sets the value of the alert level characteristic
        bool setAlertLevel(uint8_t alertLevel, bool notify);

        // Returns the current value of the alert level characteristic
        uint8_t getAlertLevel();

        // Initializes and starts everything to provide the immediate alert service
        void start();

    private:

        // UUID of Immediate Alert Service, see: https://www.bluetooth.com/specifications/gatt/services/
        static const uint16_t BLE_UUID_SERVICE_IMMEDIATE_ALERT = 0x1802;

        // UUID of Alert Level Characteristic, see: https://www.bluetooth.com/specifications/gatt/characteristics/
        static const uint16_t BLE_UUID_CHARACTERISTIC_ALERT_LEVEL = 0x2A06;

        // BLE Gatt Charcteristic: Alert Level
        static BLECharacteristic *pAlertLevelCharacteristic;
};