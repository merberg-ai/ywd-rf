# Windows development workflow

YWD-RF is intended to be easy to build and flash from a normal Windows 10/11 laptop.

## First setup

1. Install Python 3.11 or newer from python.org with the Python launcher enabled.
2. Clone the repository and switch to `dev` while developing.
3. Run `SETUP.cmd`.

`SETUP.cmd` installs PlatformIO Core in the user account if needed, downloads the ESP32 toolchain and project libraries, and performs a test build. The scripts invoke PlatformIO through `py -3 -m platformio`, so PlatformIO does not need to be manually added to the Windows PATH.

## Normal use

Double-click `YWD-RF.cmd` for the interactive menu.

Useful direct commands:

- `BUILD.cmd` — compile firmware
- `PORTS.cmd` — list serial devices
- `FLASH.cmd` — choose and flash one board
- `FLASH-BOTH.cmd` — choose and flash two boards with the same build
- `DEV.cmd` — choose board, build/flash, then open serial monitor
- `MONITOR.cmd` — serial monitor only
- `CLEAN.cmd` — clean PlatformIO build output
- `ERASE.cmd` — explicitly erase all ESP32-S3 flash contents

All scripts resolve the repository directory automatically, so they may be launched from Explorer or a shortcut.

## First YWD-RF flash after Meshtastic

A normal `FLASH.cmd` should replace the firmware. For the cleanest first bring-up, `ERASE.cmd` followed by `FLASH.cmd` removes old application/NVS contents first. Erasing is destructive, so only use it when you are ready to replace the existing firmware.

## ESP32-S3 recovery mode

If upload fails:

1. Hold the board's BOOT/PRG button.
2. Tap and release RESET/RST.
3. Release BOOT/PRG.
4. Retry `FLASH.cmd`.

Depending on the clone and USB driver, entering download mode may cause the COM port number to change. Run `PORTS.cmd` again if necessary.

## RF Lab behavior

The initial `0.0.1-dev` firmware uses a private LoRa sync word and a dedicated test frequency. Each node transmits a numbered YRF1 test frame about every five seconds with a small random jitter and listens between transmissions. The OLED and USB serial console show TX/RX counters, RSSI, SNR, duplicate count, and estimated sequence gaps.

The Heltec V3 hardware profile explicitly powers Vext for the OLED and configures the SX1262 TCXO for 1.8 V.

Do not assume the default RF Lab frequency/power is legal everywhere. Change `include/config.h` for the applicable jurisdiction and test plan before transmitting.
