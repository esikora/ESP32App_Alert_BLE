/**
    ESP32App_AlertService:
    This application has been developed to run on an M5Stack Atom Lite
    ESP32 development board. It signals an alarm that can be controlled
    by either by the Bluetooth Low Energy (BLE) service 'Immediate Alert'
    or by a WiFi TCP connection.

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

// Include either BLE or WiFi Alert Service implementation
#ifdef ALERT_SERVICE_WIFI
  #include "AlertServiceWifi.h"
  #include "WifiCredentials.h"
#endif

#ifdef ALERT_SERVICE_BLE
  #include "AlertServiceBLE.h"
#endif

// HW: Pin assignments
const uint8_t PIN_BUTTON = 39; // M5Stack Atom Lite: internal button
const uint8_t PIN_LEDATOM = 27; // M5Stack Atom Lite: internel Neopixel LED
const uint8_t PIN_GROVE_YELLOW = 32; // M5Stack Atom Lite: grove port, yellow cable

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

// Number of cycles with alarm being on
uint32_t numCyclesAlarmOn = 0;

// Toggle between alarm colors
bool ledAlarmPhase = false;

// Pointer to the alert service, either AlertServiceBLE or AlertServiceWifi
AlertService *pAlertService;

// Activates the signalling of the alarm
void activateAlarm() {

  if (state == t_State::READY) {
    numCyclesAlarmOn = 1;
    ledAlarmPhase = false;
  }

  // Retrieve the current alert level from the alert service
  uint8_t level = pAlertService->getAlertLevel();

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
  Serial.println("***** Alert Service *****");

  // Initialize the button object
  Btn.begin();

  // Attach PWM channel 0 to the GPIO pin to be controlled
  ledcAttachPin(PIN_GROVE_YELLOW, 0);
  
  // Initalize the led
  FastLED.addLeds<NEOPIXEL, PIN_LEDATOM>(ledAtom, 1);
  FastLED.clear();
  FastLED.setBrightness(brightness);
  ledAtom[0].setRGB(COLOR_READY[0], COLOR_READY[1], COLOR_READY[2]);
  FastLED.show();
  
  // Instantiate the service according to the chosen build configuration
#ifdef ALERT_SERVICE_WIFI
  pAlertService = new AlertServiceWifi(WifiCredentials::SSID, WifiCredentials::PASSWORD, WifiCredentials::PORT);
#endif

#ifdef ALERT_SERVICE_BLE
  pAlertService = new AlertServiceBLE();
#endif

  // Start the alert service
  pAlertService->start();
}

void loop() {

  // Retrieve the current alert level
  uint8_t alertLevel = pAlertService->getAlertLevel();

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
      pAlertService->setAlertLevel(0, true);
      deactivateAlarm();
    }
    // Nothing changed, keep signalling the alarm
    else {
      updateAlarm();
    }
  }

  delay(TIME_CYCLE);
}