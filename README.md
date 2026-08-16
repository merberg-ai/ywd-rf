# YWD-RF

YWD-RF is an experimental off-grid digital messenger and RF lab for ESP32-S3 + SX1262 LoRa hardware.

The project targets Heltec WiFi LoRa 32 V3-compatible boards using PlatformIO, C++/Arduino, RadioLib, and eventually a local HTML/JavaScript WebUI.

## Goals

- Reliable point-to-point text messaging over LoRa
- Delivery acknowledgements, retries, queues, and store-and-forward behavior
- Small image transfer with fragmentation and resume support
- Local phone-friendly WebUI hosted directly by the ESP32
- Useful OLED status and RF diagnostics
- Later experimentation with SX1262 GFSK for faster bulk transfers
- A documented YRF1 protocol layer that is independent from the UI

## Hardware target

Initial target:

- ESP32-S3
- Semtech SX1262
- Heltec WiFi LoRa 32 V3-compatible pinout
- 8 MB flash
- onboard OLED
- Wi-Fi / Bluetooth
- battery-powered operation

Both development boards run the same firmware. Runtime identity/configuration will distinguish the mobile and base nodes rather than maintaining separate builds.

## Branches

- `main` — stable checkpoints / known-good state
- `dev` — active development and hardware testing

## Current build: 0.0.1-dev RF Lab

The first development firmware is intentionally focused on hardware and RF validation before messaging features are layered on top.

It provides:

- SX1262 initialization through RadioLib
- private LoRa test profile
- automatic per-board node ID
- numbered bidirectional test packets
- RSSI/SNR reporting
- TX/RX/error counters
- duplicate and estimated missed-sequence counters
- OLED diagnostics
- 115200-baud USB serial logs
- Windows `.cmd` automation for setup/build/flash/monitor/recovery

The temporary RF Lab frame is documented in `protocol/YRF1.md`.

## Windows quick start

```text
git clone https://github.com/merberg-ai/ywd-rf.git
cd ywd-rf
git switch dev
SETUP.cmd
YWD-RF.cmd
```

Normal workflow:

```text
BUILD.cmd
FLASH-BOTH.cmd
```

or use `DEV.cmd` to flash one board and immediately open its serial monitor.

See `docs/WINDOWS-DEVELOPMENT.md` for details.

## Toolchain

- PlatformIO
- Arduino-ESP32
- C++
- RadioLib
- U8g2 for OLED bring-up
- HTML / CSS / JavaScript WebUI (next milestones)

## Early roadmap

1. **0.0.1-dev — RF Lab**
   - validate clone board pinout
   - SX1262 TX/RX
   - RSSI/SNR/loss statistics
   - Windows automation

2. **0.0.2-dev — YRF1 text messaging**
   - binary framing
   - node addressing
   - ACK/retry
   - message IDs / duplicate rejection

3. **0.0.3-dev — local WebUI**
   - ESP32 Wi-Fi AP
   - phone-friendly chat/history/status UI

4. **0.0.4-dev — image transfer**
   - browser-side resize/compression
   - fragmentation
   - missing-block recovery
   - resumable transfer

## RF note

`0.0.1-dev` uses a dedicated private LoRa RF Lab profile rather than Meshtastic's public channel settings. The defaults live in `include/config.h` and are intentionally easy to change. Use only frequencies, power levels, antennas, encryption, and operating practices legal for the applicable radio service and jurisdiction.

## License

Released under The Unlicense. See `LICENSE`.
