# YWD-RF

YWD-RF is an experimental off-grid digital messenger and RF lab for ESP32-S3 + SX1262 LoRa hardware.

The project is being built around Heltec WiFi LoRa 32 V3-compatible boards using PlatformIO, C++/Arduino, RadioLib, and a local HTML/JavaScript WebUI.

## Goals

- Reliable point-to-point text messaging over LoRa
- Delivery acknowledgements, retries, queues, and store-and-forward behavior
- Small image transfer with fragmentation and resume support
- Local phone-friendly WebUI hosted directly by the ESP32
- Useful OLED status and RF diagnostics
- LoRa link modes optimized for range and reliability
- Later experimentation with SX1262 GFSK for faster bulk transfers
- A documented YRF1 application/protocol layer that can grow without tying the project to one UI

## Hardware target

Initial hardware target:

- ESP32-S3
- Semtech SX1262
- Heltec WiFi LoRa 32 V3-compatible pinout
- 8 MB flash
- onboard OLED
- Wi-Fi / Bluetooth
- battery-powered operation

The two initial development nodes are `YWDM` (mobile) and `YWDB` (base). Both will run the same firmware; node identity and behavior will be runtime configuration rather than separate firmware builds.

## Toolchain

- PlatformIO
- Arduino-ESP32
- C++
- RadioLib
- HTML / CSS / JavaScript WebUI
- Windows `.cmd` automation for setup, build, flash, monitor, recovery, and release workflows

## Branches

- `main` — stable checkpoints and known-good project state
- `dev` — active development and hardware testing

Feature branches may be introduced later when useful.

## Current status

**0.0.1-dev — project bootstrap**

The initial milestone is RF Lab bring-up: verify the ESP32-S3/SX1262/OLED hardware, establish reliable bidirectional packets, and collect RSSI/SNR/loss statistics before adding the messenger protocol and WebUI.

## Planned early milestones

1. **0.0.1-dev — RF Lab**
   - board bring-up
   - SX1262 TX/RX
   - numbered test packets
   - RSSI/SNR and packet-loss statistics
   - OLED and serial diagnostics

2. **0.0.2-dev — YRF1 text messaging**
   - node addressing
   - text frames
   - ACK/retry
   - message IDs and duplicate rejection

3. **0.0.3-dev — local WebUI**
   - ESP32 Wi-Fi AP
   - phone-friendly browser UI
   - chat/history/status pages

4. **0.0.4-dev — image transfer**
   - small image upload
   - fragmentation
   - missing-block recovery
   - interrupted-transfer resume

5. **Later**
   - store-and-forward
   - multi-node operation
   - optional relay/routing
   - GFSK bulk-transfer experiments
   - OTA updates
   - integration with other YWD services

## Build

The automated Windows development workflow will live in the repository root. Until that lands on `dev`, a normal PlatformIO build is:

```text
pio run
```

The board target is PlatformIO's `heltec_wifi_lora_32_V3` environment.

## Safety / regulatory note

YWD-RF is experimental radio software. Use frequencies, power levels, antennas, encryption, and operating practices that are legal for the applicable service and jurisdiction.

## License

YWD-RF is released under The Unlicense. See `LICENSE`.
