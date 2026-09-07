#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <esp_system.h>
#include <stdarg.h>

#include "config.h"
#include "rf_lab.h"

#ifndef YWD_RF_VERSION
#define YWD_RF_VERSION "0.0.1-dev"
#endif

#ifndef YWD_RF_DIAGNOSTIC
#define YWD_RF_DIAGNOSTIC 0
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 35
#endif

using namespace ywd;

namespace {
constexpr uint32_t kDebugBaud = 115200;
constexpr int kStatusLedPin = LED_BUILTIN;
constexpr uint32_t kHeartbeatIntervalMs = 500;

uint32_t lastHeartbeatAt = 0;
bool heartbeatState = false;

void setStatusLed(bool on) {
  digitalWrite(kStatusLedPin, on ? HIGH : LOW);
}

void debugPrintf(const char* format, ...) {
  char buffer[320];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  // With ARDUINO_USB_CDC_ON_BOOT=1, Serial is the native USB CDC/JTAG
  // console. Serial0 remains UART0, which covers clone boards that put a
  // CH340/CP210x-style bridge on the USB connector instead.
  Serial.print(buffer);
  Serial0.print(buffer);
}

void initDebugChannels() {
  pinMode(kStatusLedPin, OUTPUT);

  // Earliest possible visual proof that setup() was entered. This happens
  // before either serial transport is initialized.
  setStatusLed(true);
  delay(180);
  setStatusLed(false);
  delay(80);

  // Bring up both possible development-console paths. Do not wait for either
  // one to connect; a disconnected USB CDC host must never stall boot.
  Serial0.begin(kDebugBaud);
  Serial.begin(kDebugBaud);
  Serial.setDebugOutput(true);
  delay(100);
}

void pulseBootStage(uint8_t stage) {
  // One-time visual breadcrumbs during boot. If serial is unavailable, reset
  // the board and count the pulses to see the last stage reached.
  for (uint8_t i = 0; i < stage; ++i) {
    setStatusLed(true);
    delay(90);
    setStatusLed(false);
    delay(110);
  }
  delay(250);
}

[[noreturn]] void fatalBlink(uint8_t code) {
  // Repeating LED error code. A long gap separates groups.
  while (true) {
    for (uint8_t i = 0; i < code; ++i) {
      setStatusLed(true);
      delay(180);
      setStatusLed(false);
      delay(180);
    }
    delay(1400);
  }
}

void updateHeartbeat() {
  const uint32_t now = millis();
  if (now - lastHeartbeatAt >= kHeartbeatIntervalMs) {
    lastHeartbeatAt = now;
    heartbeatState = !heartbeatState;
    setStatusLed(heartbeatState);
  }
}

void printBootDiagnostics() {
  debugPrintf("\n================================================\n");
  debugPrintf(" YWD-RF %s - boot diagnostics\n", YWD_RF_VERSION);
  debugPrintf("================================================\n");
  debugPrintf("[BOOT] reset reason: %d\n", static_cast<int>(esp_reset_reason()));
  debugPrintf("[BOOT] CPU: %u MHz\n", ESP.getCpuFreqMHz());
  debugPrintf("[BOOT] flash: %u bytes\n", ESP.getFlashChipSize());
  debugPrintf("[BOOT] free heap: %u bytes\n", ESP.getFreeHeap());
  debugPrintf("[BOOT] USB CDC console: Serial\n");
  debugPrintf("[BOOT] UART0 console: Serial0 @ %lu baud\n",
              static_cast<unsigned long>(kDebugBaud));
#if YWD_RF_DIAGNOSTIC
  debugPrintf("[BOOT] mode: MINIMAL DIAGNOSTIC (OLED/RADIO/SPI disabled)\n");
#else
  debugPrintf("[BOOT] mode: RF LAB\n");
#endif
}
}  // namespace

String nodeId;

#if !YWD_RF_DIAGNOSTIC
SPIClass loraSpi(FSPI);
SPISettings loraSpiSettings(2000000, MSBFIRST, SPI_MODE0);
SX1262 radio = new Module(
    PIN_LORA_NSS,
    PIN_LORA_DIO1,
    PIN_LORA_RST,
    PIN_LORA_BUSY,
    loraSpi,
    loraSpiSettings);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0,
    PIN_OLED_RST,
    PIN_OLED_SCL,
    PIN_OLED_SDA);

RfStats stats;
String lastSource = "-";
uint32_t txSequence = 0;
uint32_t nextTxAt = 0;
uint32_t lastDisplayAt = 0;
volatile bool receivedFlag = false;
bool radioOk = false;
bool displayOk = false;

void setFlag() { receivedFlag = true; }

void resetOled() {
  // Heltec V3-class boards power the OLED from Vext and expose the SSD1306
  // reset line on GPIO21. Explicitly release/reset it before any I2C access.
  pinMode(PIN_OLED_RST, OUTPUT);
  digitalWrite(PIN_OLED_RST, HIGH);
  delay(1);
  digitalWrite(PIN_OLED_RST, LOW);
  delay(20);
  digitalWrite(PIN_OLED_RST, HIGH);
  delay(20);
}

bool oledAddressResponds() {
  Wire.beginTransmission(OLED_ADDR);
  const uint8_t result = Wire.endTransmission(true);
  debugPrintf("[BOOT:3] OLED probe 0x%02X result=%u\n", OLED_ADDR, result);
  return result == 0;
}

void showBoot(const char* line1, const char* line2 = "") {
  if (!displayOk) return;
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(0, 12, "YWD-RF RF LAB");
  display.drawStr(0, 28, line1);
  display.drawStr(0, 42, line2);
  display.sendBuffer();
}

void drawStatus() {
  if (!displayOk) return;

  display.clearBuffer();
  display.setFont(u8g2_font_5x8_tf);

  char line[64];
  snprintf(line, sizeof(line), "YWD-RF %s", YWD_RF_VERSION);
  display.drawStr(0, 8, line);

  snprintf(line, sizeof(line), "%s  %.3f MHz", nodeId.c_str(), RF_FREQUENCY_MHZ);
  display.drawStr(0, 18, line);

  snprintf(line, sizeof(line), "TX:%lu RX:%lu ERR:%lu",
           static_cast<unsigned long>(stats.txCount),
           static_cast<unsigned long>(stats.rxCount),
           static_cast<unsigned long>(stats.crcOrRxErrors));
  display.drawStr(0, 30, line);

  snprintf(line, sizeof(line), "MISS:%lu DUP:%lu",
           static_cast<unsigned long>(stats.missedSequenceEstimate),
           static_cast<unsigned long>(stats.duplicateCount));
  display.drawStr(0, 40, line);

  if (stats.rxCount > 0) {
    snprintf(line, sizeof(line), "RSSI:%4.0f SNR:%4.1f", stats.lastRssi, stats.lastSnr);
    display.drawStr(0, 50, line);
    snprintf(line, sizeof(line), "LAST:%s #%lu", lastSource.c_str(),
             static_cast<unsigned long>(stats.lastRxSequence));
  } else {
    snprintf(line, sizeof(line), "Listening SF%u BW%.0f", RF_SPREADING_FACTOR, RF_BANDWIDTH_KHZ);
  }
  display.drawStr(0, 61, line);
  display.sendBuffer();
}

void startReceive() {
  const int16_t state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    debugPrintf("[RF] startReceive failed: %d\n", state);
    stats.crcOrRxErrors++;
  }
}

void scheduleNextTx() {
  // A little random jitter prevents two boards booted together from repeatedly
  // transmitting on top of each other forever.
  const uint32_t jitter = esp_random() % 501;
  nextTxAt = millis() + TEST_TX_INTERVAL_MS + jitter;
}

void transmitTestPacket() {
  String payload = makeTestPayload(nodeId, ++txSequence);
  debugPrintf("[TX] #%lu %s\n", static_cast<unsigned long>(txSequence), payload.c_str());

  radio.clearDio1Action();
  const int16_t state = radio.transmit(payload);
  if (state == RADIOLIB_ERR_NONE) {
    stats.txCount++;
    debugPrintf("[TX] OK  %.1f ms airtime\n", radio.getTimeOnAir(payload.length()) / 1000.0f);
  } else {
    debugPrintf("[TX] ERROR %d\n", state);
    stats.crcOrRxErrors++;
  }

  radio.setDio1Action(setFlag);
  startReceive();
  scheduleNextTx();
}

void handleReceive() {
  receivedFlag = false;

  String payload;
  const int16_t state = radio.readData(payload);
  if (state != RADIOLIB_ERR_NONE) {
    debugPrintf("[RX] ERROR %d\n", state);
    stats.crcOrRxErrors++;
    startReceive();
    return;
  }

  stats.lastRssi = radio.getRSSI();
  stats.lastSnr = radio.getSNR();
  stats.lastRxAtMs = millis();

  String source;
  uint32_t sequence = 0;
  if (!parseTestPayload(payload, source, sequence)) {
    debugPrintf("[RX] FOREIGN/UNKNOWN RSSI %.1f SNR %.1f: %s\n",
                stats.lastRssi, stats.lastSnr, payload.c_str());
    startReceive();
    return;
  }

  if (source == nodeId) {
    debugPrintf("[RX] self packet ignored #%lu\n", static_cast<unsigned long>(sequence));
    startReceive();
    return;
  }

  stats.rxCount++;
  lastSource = source;

  if (stats.haveLastRxSequence) {
    if (sequence == stats.lastRxSequence) {
      stats.duplicateCount++;
    } else if (sequence > stats.lastRxSequence + 1) {
      stats.missedSequenceEstimate += sequence - stats.lastRxSequence - 1;
    }
  }

  stats.lastRxSequence = sequence;
  stats.haveLastRxSequence = true;

  debugPrintf("[RX] %s #%lu RSSI %.1f dBm SNR %.1f dB\n",
              source.c_str(), static_cast<unsigned long>(sequence),
              stats.lastRssi, stats.lastSnr);

  startReceive();
}
#endif

void setup() {
  initDebugChannels();
  pulseBootStage(1);
  printBootDiagnostics();

  nodeId = makeNodeId();
  debugPrintf("[BOOT:1] application entered, node %s\n", nodeId.c_str());

#if YWD_RF_DIAGNOSTIC
  // This image proves the ESP32 application can boot without even constructing
  // the board-specific radio/display objects. A 2 Hz LED heartbeat plus text
  // on USB CDC or UART0 means the CPU/runtime are alive.
  debugPrintf("[DIAG] Peripheral objects and initialization are disabled.\n");
  debugPrintf("[DIAG] Expect the status LED to toggle every 500 ms.\n");
  debugPrintf("[DIAG] If this works, the failure is later in OLED/SPI/SX1262 bring-up.\n");
  return;
#else
  pulseBootStage(2);
  debugPrintf("[BOOT:2] enabling Vext/OLED power on GPIO %d\n", PIN_VEXT);

  // Heltec V3 Vext is active-low and powers the onboard OLED rail.
  pinMode(PIN_VEXT, OUTPUT);
  digitalWrite(PIN_VEXT, LOW);
  delay(100);

  pulseBootStage(3);
  debugPrintf("[BOOT:3] resetting OLED on GPIO %d\n", PIN_OLED_RST);
  resetOled();

  debugPrintf("[BOOT:3] starting I2C SDA=%d SCL=%d @ 100 kHz timeout=50 ms\n",
              PIN_OLED_SDA, PIN_OLED_SCL);
  Wire.setTimeOut(50);
  const bool wireOk = Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL, 100000);
  debugPrintf("[BOOT:3] Wire.begin returned %s\n", wireOk ? "true" : "false");

  if (wireOk && oledAddressResponds()) {
    debugPrintf("[BOOT:3] OLED ACKed; calling U8g2 begin()\n");
    display.setI2CAddress(OLED_ADDR << 1);
    display.begin();
    displayOk = true;
    showBoot("Starting...", nodeId.c_str());
    debugPrintf("[BOOT:3] OLED initialization returned successfully\n");
  } else {
    debugPrintf("[WARN] OLED unavailable; continuing RF Lab headless.\n");
  }

  pulseBootStage(4);
  debugPrintf("[BOOT:4] starting LoRa SPI SCK=%d MISO=%d MOSI=%d NSS=%d\n",
              PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_NSS);
  loraSpi.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_NSS);

  pulseBootStage(5);
  debugPrintf("[BOOT:5] initializing SX1262 DIO1=%d RST=%d BUSY=%d\n",
              PIN_LORA_DIO1, PIN_LORA_RST, PIN_LORA_BUSY);
  showBoot("Initializing SX1262", nodeId.c_str());
  radio.tcxoVoltage = RF_TCXO_VOLTAGE;

  const int16_t state = radio.begin(
      RF_FREQUENCY_MHZ,
      RF_BANDWIDTH_KHZ,
      RF_SPREADING_FACTOR,
      RF_CODING_RATE,
      RF_SYNC_WORD,
      RF_TX_POWER_DBM,
      RF_PREAMBLE_LEN,
      RF_TCXO_VOLTAGE,
      false);

  if (state != RADIOLIB_ERR_NONE) {
    debugPrintf("[FATAL:6] SX1262 begin failed: %d\n", state);
    const String code = String(state);
    showBoot("SX1262 FAILED", code.c_str());
    fatalBlink(6);
  }

  radioOk = true;
  radio.setDio1Action(setFlag);
  startReceive();
  scheduleNextTx();

  debugPrintf("[BOOT:6] SX1262 initialized successfully\n");
  debugPrintf("[BOOT:6] OLED status: %s\n", displayOk ? "ONLINE" : "HEADLESS");
  debugPrintf("[RF] SX1262 OK %.3f MHz SF%u BW%.0f CR4/%u TX %d dBm TCXO %.1f V\n",
              RF_FREQUENCY_MHZ, RF_SPREADING_FACTOR, RF_BANDWIDTH_KHZ,
              RF_CODING_RATE, RF_TX_POWER_DBM, RF_TCXO_VOLTAGE);
  debugPrintf("[RF] Each node sends a numbered test packet about every 5 seconds.\n");
  debugPrintf("[RF] Move the nodes apart and watch RSSI/SNR/missed counters.\n");

  drawStatus();
#endif
}

void loop() {
  updateHeartbeat();

#if YWD_RF_DIAGNOSTIC
  static uint32_t lastDiagnosticAt = 0;
  const uint32_t now = millis();
  if (now - lastDiagnosticAt >= 2000) {
    lastDiagnosticAt = now;
    debugPrintf("[DIAG] alive uptime=%lu ms heap=%u reset=%d\n",
                static_cast<unsigned long>(now),
                ESP.getFreeHeap(),
                static_cast<int>(esp_reset_reason()));
  }
  delay(5);
  return;
#else
  if (!radioOk) return;

  if (receivedFlag) {
    handleReceive();
  }

  const uint32_t now = millis();
  if (static_cast<int32_t>(now - nextTxAt) >= 0) {
    transmitTestPacket();
  }

  if (now - lastDisplayAt >= DISPLAY_REFRESH_MS) {
    lastDisplayAt = now;
    drawStatus();
  }

  delay(5);
#endif
}
