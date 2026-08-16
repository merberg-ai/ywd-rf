# Changelog

## 0.0.1-dev

### Added
- Initial PlatformIO project for Heltec WiFi LoRa 32 V3-compatible ESP32-S3/SX1262 boards.
- RadioLib SX1262 bring-up and private LoRa RF Lab profile.
- Automatic per-device `YWD-xxxx` node identifier based on ESP32 eFuse MAC.
- Numbered `YRF1|TEST|...` test packets sent every five seconds.
- Receive-side RSSI/SNR, duplicate and estimated-missed-sequence counters.
- OLED RF Lab status display and detailed USB serial diagnostics.
- Windows setup/build/flash/monitor/erase automation and interactive `YWD-RF.cmd` menu.
- Initial YRF1 protocol notes and Windows development documentation.

### Planned next
- Confirm pinout and RF behavior on the two clone boards.
- Add persistent runtime node naming.
- Replace temporary text test frames with binary YRF1 framing.
- Add ACK/retry and real text messaging.
