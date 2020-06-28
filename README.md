# ESP32App_AlertService
This application has been developed to run on an M5Stack Atom Lite ESP32 development board. It signals an alarm that can be activated by a remote client application. Adapters for two different types of clients are provided:
- Bluetooth Low Energy (BLE)
- WiFi TCP Connection

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
- WiFi

Configuration via `platformio.ini`:
- `default_envs = pico32_ble` or
- `default_envs = pico32_wifi`

## Project Description
A description of this project, the hardware used, and some notes on how to test the ESP32 application using a smartphone app are provided [here](https://www.hackster.io/esikora/alarm-device-with-esp32-atom-lite-and-bluetooth-low-energy-b4887f).

## License

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

See the [LICENSE](LICENSE) file for details.

Copyright 2020 © Ernst Sikora
