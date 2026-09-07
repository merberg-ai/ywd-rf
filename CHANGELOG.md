# Changelog

## 0.0.1-dev

### Added
- Initial PlatformIO project for Heltec WiFi LoRa 32 V3-compatible ESP32-S3/SX1262 boards.
- RadioLib SX1262 bring-up and private LoRa RF Lab profile.
- Automatic per-device `YWD-xxxx` node identifier based on ESP32 eFuse MAC.
- Numbered `YRF1|TEST|...` test packets sent every five seconds.
- Receive-side RSSI/SNR, duplicate and estimated-missed-sequence counters.
- OLED RF Lab status display and detailed USB serial diagnostics.
- Dual debug output over native USB CDC and UART0, LED boot breadcrumbs, a minimal diagnostic image, and a staged peripheral-probe image.
- Windows setup/build/flash/monitor/erase automation and interactive `YWD-RF.cmd` menu.
- Initial YRF1 protocol notes and Windows development documentation.

### Fixed
- Explicit Heltec V3 OLED power/reset sequencing before I2C access.
- ESP32-S3 reboot during `U8g2::begin()` caused by handing U8g2 an already-active `Wire` bus. The RF Lab now probes the SSD1306, calls `Wire.end()`, and then lets U8g2 take clean ownership with reset disabled because reset is handled manually.
- OLED failure can no longer prevent SX1262 bring-up; RF Lab continues headless when the display is unavailable.

### Verified on hardware
- ESP32-S3 application boot, 8 MB flash, native USB serial, and stable heap.
- SSD1306 power/reset path and I2C response at address `0x3C`.
- SX1262 SPI/control pinout, RadioLib initialization (`state=0`), and standby operation without RF transmit.

### Planned next
- Confirm bidirectional RF Lab packets between the two development nodes and collect RSSI/SNR/loss data.
- Add persistent runtime node naming.
- Replace temporary text test frames with binary YRF1 framing.
- Add ACK/retry and real text messaging.
