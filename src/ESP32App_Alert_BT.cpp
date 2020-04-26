/**
    ESP32App_Alert_BT:
    This application has been developed to run on an M5Stack Atom Lite
    ESP32 development board. It signals an alarm that is controlled by
    the Bluetooth Low Energy (BLE) service 'Immediate Alert'.

    Copyright (C) 2020 by Ernst Sikora
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>

// External library: JC_Button, https://github.com/JChristensen/JC_Button
#include <JC_Button.h>

// External library: FastLED, https://github.com/FastLED/FastLED
#include <FastLED.h> 

// Library: ESP32 BLE Arduino
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// UUID of Immediate Alert Service, see: https://www.bluetooth.com/specifications/gatt/services/
const uint16_t BLE_UUID_SERVICE_IMMEDIATE_ALERT = 0x1802;

// UUID of Alert Level Characteristic, see: https://www.bluetooth.com/specifications/gatt/characteristics/
const uint16_t BLE_UUID_CHARACTERISTIC_ALERT_LEVEL = 0x2A06;

// HW: Pin assignments
const byte PIN_BUTTON = 39; // M5Stack Atom Lite: internal button
const byte PIN_LEDATOM = 27; // M5Stack Atom Lite: internel Neopixel LED

// Status LED: color definitions
const uint8_t COLOR_READY[3]  = {0, 20, 0}; // System state: READY
const uint8_t COLOR_ALARM[3]  = {100, 0, 0}; // System state: ALARM
const uint8_t COLOR_ALARM2[3] = {100, 100, 0}; // System state: ALARM

// Number of cycles to switch between alarm colors
const uint8_t ALARM_BLINK_NUM_CYCLES = 10;

// Number of cycles after which alarm can be switched off
const uint8_t ALARM_MIN_NUM_CYCLES = 20;

// Cycle time of main loop
const int TIME_CYCLE = 50; // ms

// Type declarations
enum State {READY = 0, ALARM = 1}; // Main system states
typedef enum State t_State;

// System state
t_State state = t_State::READY;

// Internal button
Button Btn(PIN_BUTTON);

// Internal LED controller
CRGB ledAtom[1];

// Brightness factor for LED
uint8_t brightness = 255;

// BLE Gatt Charcteristic: Alarm Level
BLECharacteristic *pAlertLevelCharacteristic = 0;

// Number of cycles with alarm being on
uint32_t numCyclesAlarmOn = 0;

// Toggle between alarm colors
bool ledAlarmColor2 = false;

// Sets the value of the alarm level characteristic
void setAlertLevel(uint8_t alertLevel, bool notify) {
  pAlertLevelCharacteristic->setValue(&alertLevel, 1);
  
  if (notify) {
    pAlertLevelCharacteristic->notify(); // Notify client about the change
  }
}

// Returns the value of the alert level characteristic
uint8_t getAlertLevel() {
  std::string data = pAlertLevelCharacteristic->getValue();

  if (data.length() == 1) {
    uint8_t alarmLevel = data[0];

    if (alarmLevel >= 0 && alarmLevel <= 2) {
      return alarmLevel;
    }
    else {
      return 255; // Value error
    }
  }
  else {
    return 255; // Data length error
  }
}

// Activates the signalling of the alarm
void activateAlarm() {
  if (state == t_State::READY) {
    numCyclesAlarmOn = 1;
    state = t_State::ALARM;

    ledAlarmColor2 = false;

    ledAtom[0].setRGB(COLOR_ALARM[0], COLOR_ALARM[1], COLOR_ALARM[2]);
    FastLED.show();
  }
}

// Deactivates the signalling of the alarm
void deactivateAlarm() {
  if (state == t_State::ALARM) {
    numCyclesAlarmOn = 0;
    state = t_State::READY;

    ledAlarmColor2 = false;

    ledAtom[0].setRGB(COLOR_READY[0], COLOR_READY[1], COLOR_READY[2]);
    FastLED.show();
  }
}

// Updates the alarm output
void updateAlarm() {
  if (numCyclesAlarmOn % ALARM_BLINK_NUM_CYCLES == 0)
  {
    ledAlarmColor2 = !ledAlarmColor2;

    if (ledAlarmColor2) {
      ledAtom[0].setRGB(COLOR_ALARM2[0], COLOR_ALARM2[1], COLOR_ALARM2[2]);
      FastLED.show();
    }
    else {
      ledAtom[0].setRGB(COLOR_ALARM[0], COLOR_ALARM[1], COLOR_ALARM[2]);
      FastLED.show();
    }
  }

  numCyclesAlarmOn += 1;
}

void setup() {
  Serial.begin(115200);
  Serial.println("***** BLE Immediate Alert Service *****");

  // Initialize the button object
  Btn.begin();
  
  // Initalize the led
  FastLED.addLeds<NEOPIXEL, PIN_LEDATOM>(ledAtom, 1);
  FastLED.clear();
  FastLED.setBrightness(brightness);
  ledAtom[0].setRGB(COLOR_READY[0], COLOR_READY[1], COLOR_READY[2]);
  FastLED.show();

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

void loop() {

  // Retrieve the current alert level from the BLE alert level characteristic  
  uint8_t alertLevel = getAlertLevel();

  // Read the button state
  Btn.read();

  // Activate alarm if alert level is greater than zero, deactivate it otherwise
  switch (state) {

    case t_State::READY:
      // Activation of alarm by BLE client
      if (alertLevel > 0) {
        activateAlarm();
      }
      break;

    case t_State::ALARM:
      // Deactivation of alarm by BLE client
      if (alertLevel == 0) {
        deactivateAlarm();
      }
      else {
        // Deactivation of alarm by user interaction
        if (Btn.wasReleased() && numCyclesAlarmOn >= ALARM_MIN_NUM_CYCLES)
        {
          setAlertLevel(0, true);
          deactivateAlarm();
        }
        else {
          // Keep signalling the alarm
          updateAlarm();
        }
      }
  }

  delay(TIME_CYCLE);
}