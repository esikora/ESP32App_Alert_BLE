# ESP32App_Alert_BLE
This application has been developed to run on an M5Stack Atom Lite ESP32 development board. It signals an alarm and is controlled by the Bluetooth Low Energy (BLE) "Immediate Alert" service.

## Getting Started
Development environment used for this application:
- Visual Studio Code (version 1.44.2)
- PlatformIO IDE for VSCode

Platform and board:
- ESP32 Pico

Libraries used (see platformio.ini):
- JC_Button
- FastLED
- BLE ESP32 Arduino

## Project Description

To test the ESP32 application a BLE client is needed. For this purpose, I used the [nRF Connect for mobile app](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Connect-for-mobile).

Within the app the following steps are needed:
- Tab "SCANNER": Activate "SCAN" and connect to the "ESP32_Alert" device.
- Tab "ESP32_Alert": Unfold the "Immediate Alert" Service. Activate notifications. Write value "0x01 (Mild alert)" and "send".

After the ESP32 signals the alarm, press the button in order to deactivate the alarm.
The nRF Connect app then shows the value "No Alert" again.

## License

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

See the [LICENSE.md](LICENSE.md) file for details.

Copyright 2020 © Ernst Sikora
