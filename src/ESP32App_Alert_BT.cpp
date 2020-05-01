/**
    ESP32App_Alert_BLE:
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
const byte PIN_GROVE_YELLOW = 32; // M5Stack Atom Lite: grove port, yellow cable

// Status LED: color definitions
const uint8_t COLOR_READY[3]  = {0, 10, 0}; // System state: READY
const uint8_t COLOR_ALARM_MILD[2][3] = { {150, 150, 0}, { 75,  75, 0} }; // System state: Alarm Mild, phases 1 & 2
const uint8_t COLOR_ALARM_HIGH[2][3] = { {200,   0, 0}, {150, 100, 0} }; // System state: Alarm High, phases 1 & 2

// Number of cycles to switch between alarm colors
const uint8_t ALARM_BLINK_NUM_CYCLES = 10;

// Number of cycles after which alarm can be switched off
const uint8_t ALARM_MIN_NUM_CYCLES = 20;

// Frequency of vibration alarm signal for cases "No Alert", "Mild Alert", and "High Alert"
const double ALARM_FREQ[3] = {0, 1, 2};

// Cycle time of main loop
const int TIME_CYCLE = 50; // ms

// Type declarations
enum State {READY = 0, ALARM_MILD = 1, ALARM_HIGH = 2}; // Main system states
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
bool ledAlarmPhase = false;

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
    ledAlarmPhase = false;
  }

  uint8_t level = getAlertLevel();

  switch (level) {
    case 1:
      state = t_State::ALARM_MILD;
      ledAtom[0].setRGB(COLOR_ALARM_MILD[0][0], COLOR_ALARM_MILD[0][1], COLOR_ALARM_MILD[0][2]);
      break;

    case 2:
      state = t_State::ALARM_HIGH;
      ledAtom[0].setRGB(COLOR_ALARM_HIGH[0][0], COLOR_ALARM_HIGH[0][1], COLOR_ALARM_HIGH[0][2]);
      break;
  }

  // Activate vibration alarm with desired frequency on PWM channel 0
  ledcWriteTone(0, ALARM_FREQ[level]);

  FastLED.show();
}

// Deactivates the signalling of the alarm
void deactivateAlarm() {
  numCyclesAlarmOn = 0;
  state = t_State::READY;

  // Deactivate vibration alarm on PWM channel 0
  ledcWrite(0, 0);

  // Change color of internal Led
  ledAlarmPhase = false;
  ledAtom[0].setRGB(COLOR_READY[0], COLOR_READY[1], COLOR_READY[2]);
  FastLED.show();
}

// Updates the alarm output
void updateAlarm() {
  if (numCyclesAlarmOn % ALARM_BLINK_NUM_CYCLES == 0) {
    ledAlarmPhase = !ledAlarmPhase;

    if (state == t_State::ALARM_MILD) {
      ledAtom[0].setRGB(COLOR_ALARM_MILD[ledAlarmPhase][0], COLOR_ALARM_MILD[ledAlarmPhase][1], COLOR_ALARM_MILD[ledAlarmPhase][2]);
    }
    else if (state == t_State::ALARM_HIGH) {
        ledAtom[0].setRGB(COLOR_ALARM_HIGH[ledAlarmPhase][0], COLOR_ALARM_HIGH[ledAlarmPhase][1], COLOR_ALARM_HIGH[ledAlarmPhase][2]);
    }
    FastLED.show();
  }

  numCyclesAlarmOn += 1;
}

void setup() {
  Serial.begin(115200);
  Serial.println("***** BLE Immediate Alert Service *****");

  // Initialize the button object
  Btn.begin();

  // Initialize alarm output pin
  //pinMode(PIN_GROVE_YELLOW, OUTPUT);

  // configure LED PWM functionalitites
  //ledcSetup(0, 1, 10); // Channel 0, 1 Hz, 10 Bit resolution
  
  // Attach PWM channel 0 to the GPIO pin to be controlled
  ledcAttachPin(PIN_GROVE_YELLOW, 0);
  
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
  if (state == t_State::READY) {
    // Activation of alarm by BLE client
    if (alertLevel > 0) {
      activateAlarm();
    }
  }
  else {
    // Change of alert level from mild to high
    if (state == t_State::ALARM_MILD && alertLevel == 2) {
      activateAlarm();
    }
    // Change of alert level from high to mild
    else if (state == t_State::ALARM_HIGH && alertLevel == 1) {
      activateAlarm();
    }
    // Deactivation of alarm by BLE client
    else if (alertLevel == 0) {
      deactivateAlarm();
    }
    // Deactivation of alarm by user interaction
    else if (Btn.wasReleased() && numCyclesAlarmOn >= ALARM_MIN_NUM_CYCLES)
    {
      setAlertLevel(0, true);
      deactivateAlarm();
    }
    // Nothing changed, keep signalling the alarm
    else {
      updateAlarm();
    }
  }

  delay(TIME_CYCLE);
}