#include "AlertServiceBLE.h"

BLECharacteristic *AlertServiceBLE::pAlertLevelCharacteristic = nullptr;

bool AlertServiceBLE::setAlertLevel(uint8_t alertLevel, bool notify) {
    if (pAlertLevelCharacteristic != nullptr) {
        pAlertLevelCharacteristic->setValue(&alertLevel, 1);
        
        if (notify) {
            pAlertLevelCharacteristic->notify(); // Notify client about the change
        }
        return true;
    }
    else {
        return false; // Not initialized
    }
}

uint8_t AlertServiceBLE::getAlertLevel() {
    if (pAlertLevelCharacteristic != nullptr) {
        std::string data = pAlertLevelCharacteristic->getValue();

        if (data.length() == 1) {
            uint8_t alertLevel = data[0];

            if ( isValidLevel(alertLevel) ) {
                return alertLevel;
            }
            else {
                return 255; // Value error
            }
        }
        else {
            return 255; // Data length error
        }
    }
    else {
        return 255; // Initialization error
    }
}

void AlertServiceBLE::start() {
    if (pAlertLevelCharacteristic == nullptr) {
        // Initialize bluetooth device
        BLEDevice::init("ESP32_Alert");
        BLEDevice::setPower(ESP_PWR_LVL_P9);
        
        // Create BLE GATT server
        BLEServer *pServer = BLEDevice::createServer();

        // Create BLE GATT service "Immediate Alert"
        BLEService *pService = pServer->createService(BLE_UUID_SERVICE_IMMEDIATE_ALERT);

        // Create BLE GATT characteristic "Alert Level"
        pAlertLevelCharacteristic = pService->createCharacteristic(
                                        BLE_UUID_CHARACTERISTIC_ALERT_LEVEL,
                                        BLECharacteristic::PROPERTY_READ |
                                        BLECharacteristic::PROPERTY_WRITE |
                                        BLECharacteristic::PROPERTY_WRITE_NR |
                                        BLECharacteristic::PROPERTY_NOTIFY
                                    );

        // Create a BLE Descriptor to support notification of client by the server
        // see: https://www.bluetooth.com/specifications/gatt/descriptors/
        pAlertLevelCharacteristic->addDescriptor(new BLE2902());

        // Initial alert level is "No alert"
        setAlertLevel(0, false);
        
        // pAlertLevelCharacteristic->setCallbacks(new AlertLevelCallback());
        
        // Start the service
        pService->start();

        // Start advertising
        BLEAdvertising *pAdvertising = pServer->getAdvertising();
        pAdvertising->addServiceUUID(BLE_UUID_SERVICE_IMMEDIATE_ALERT); // Advertise the immediate alert service
        pAdvertising->start();
    }
}