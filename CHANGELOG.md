# Changelog

## 0.0.1-dev

### Added
- Initial PlatformIO project for Heltec WiFi LoRa 32 V3-compatible ESP32-S3/SX1262 boards.
- RadioLib SX1262 bring-up and private LoRa RF Lab profile.
- Automatic per-device `YWD-xxxxxx` node identifier derived from the ESP32 eFuse MAC.
- Numbered `YRF1|TEST|...` test packets with randomized transmit spacing.
- Receive-side RSSI/SNR, raw packet, duplicate and estimated-missed-sequence counters.
- Runtime RF qualification modes over serial: `AUTO`, `TXONLY`, and `RXONLY`, plus detailed status/help commands.
- Runtime `POWER` command that sleeps the SX1262, powers down the OLED/Vext rail, and puts the ESP32-S3 into deep sleep until reset or power cycle.
- DIO1 polling fallback and RX-arm telemetry so receive-path failures cannot hide behind an interrupt/callback problem.
- OLED RF Lab status display and detailed USB serial diagnostics.
- Dual debug output over native USB CDC and UART0, LED boot breadcrumbs, a minimal diagnostic image, and a staged peripheral-probe image.
- Windows setup/build/flash/monitor/erase automation and interactive `YWD-RF.cmd` menu.
- Initial YRF1 protocol notes and Windows development documentation.

### Fixed
- Explicit Heltec V3 OLED power/reset sequencing before I2C access.
- ESP32-S3 reboot during `U8g2::begin()` caused by handing U8g2 an already-active `Wire` bus. The RF Lab now probes the SSD1306, calls `Wire.end()`, and then lets U8g2 take clean ownership with reset disabled because reset is handled manually.
- OLED failure can no longer prevent SX1262 bring-up; RF Lab continues headless when the display is unavailable.
- RF Lab node IDs were widened from a 16-bit MAC suffix to a folded 24-bit identifier to reduce accidental self-ID collisions during multi-node testing.
- Sequence tracking now compares a received sequence against the previous value before updating the stored sequence.

### Verified on hardware
- ESP32-S3 application boot, 8 MB flash, native USB serial, and stable heap.
- SSD1306 power/reset path and I2C response at address `0x3C`.
- SX1262 SPI/control pinout, RadioLib initialization (`state=0`), and standby operation without RF transmit.
- Both development boards boot the RF Lab firmware, transmit successfully, and receive each other's YRF1 test packets bidirectionally.
- Runtime power-down command successfully sleeps the radio, shuts down the display/Vext rail, and leaves the ESP32-S3 in deep sleep until reset or power cycle.

### Planned next
- Run a sustained bidirectional RF Lab session and collect RSSI/SNR/loss data before freezing the 0.0.1-dev hardware baseline.
- Add persistent runtime node naming.
- Replace temporary text test frames with binary YRF1 framing.
- Add ACK/retry and real text messaging.
