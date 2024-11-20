# ESP32 Keyboard

This project utilizes the ESP32-S3 to transform a wired USB keyboard into a wireless keyboard using USB OTG capabilities and HID Bluetooth.

## Overview

The ESP32-S3 microcontroller is leveraged to enable USB OTG (On-The-Go) functionality, allowing it to act as a host for USB devices. This project specifically focuses on converting a standard wired USB keyboard into a wireless keyboard that communicates via Bluetooth HID (Human Interface Device).

## Features

- **USB OTG Support**: Enables the ESP32-S3 to interface with USB keyboards, supporting auto-repeat key presses, modifier keys and toggling keyboard LEDs.
- **Bluetooth HID**: Transforms the USB keyboard input into Bluetooth signals, making it compatible with various Bluetooth-enabled devices.


## Getting Started

### Usage

1. Connect the wired USB keyboard to the ESP32-S3 board by placing the USB D- (white) into GPIO pin 19 and USB D+ (green) into GPIO pin 20.
2. Upload the firmware to the ESP32-S3.
3. Pair the ESP32-S3 with your Bluetooth-enabled device.
4. Start typing on the wired USB keyboard, and the input will be transmitted wirelessly via Bluetooth.

## Credits

This project is built upon the following libraries:

- [EspUsbHost](https://github.com/tanakamasayuki/EspUsbHost): Provides USB host functionality for the ESP32.
- [ESP32-BLE-Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard): Facilitates Bluetooth HID communication for the ESP32.

Special thanks to the authors of the EspUsbHost and ESP32-BLE-Keyboard libraries.