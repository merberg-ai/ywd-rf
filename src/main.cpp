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
constexpr uint32_t kTxJitterMs = 4000;

uint32_t lastHeartbeatAt = 0;
bool heartbeatState = false;

void setStatusLed(bool on) {
  digitalWrite(kStatusLedPin, on ? HIGH : LOW);
}

void debugPrintf(const char* format, ...) {
  char buffer[384];
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

  Serial0.begin(kDebugBaud);
  Serial.begin(kDebugBaud);
  Serial.setDebugOutput(true);
  delay(100);
}

void pulseBootStage(uint8_t stage) {
  for (uint8_t i = 0; i < stage; ++i) {
    setStatusLed(true);
    delay(90);
    setStatusLed(false);
    delay(110);
  }
  delay(250);
}

[[noreturn]] void fatalBlink(uint8_t code) {
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

// OLED reset is handled explicitly before probing. Giving U8g2 reset=NONE is
// important on ESP32-S3: the probe temporarily owns Wire, then Wire.end() is
// called before U8g2 takes ownership and initializes the bus exactly once.
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0,
    U8X8_PIN_NONE,
    PIN_OLED_SCL,
    PIN_OLED_SDA);

enum class LabMode : uint8_t {
  Auto,
  TxOnly,
  RxOnly,
};

RfStats stats;
String lastSource = "-";
uint32_t txSequence = 0;
uint32_t nextTxAt = 0;
uint32_t lastDisplayAt = 0;
volatile bool receivedFlag = false;
bool radioOk = false;
bool displayOk = false;
bool receivingArmed = false;
LabMode labMode = LabMode::Auto;

const char* labModeName() {
  switch (labMode) {
    case LabMode::TxOnly: return "TXONLY";
    case LabMode::RxOnly: return "RXONLY";
    default: return "AUTO";
  }
}

void setFlag() { receivedFlag = true; }

void resetOled() {
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

  snprintf(line, sizeof(line), "%s %s", nodeId.c_str(), labModeName());
  display.drawStr(0, 18, line);

  snprintf(line, sizeof(line), "TX:%lu RAW:%lu RX:%lu",
           static_cast<unsigned long>(stats.txCount),
           static_cast<unsigned long>(stats.rawRxCount),
           static_cast<unsigned long>(stats.rxCount));
  display.drawStr(0, 30, line);

  snprintf(line, sizeof(line), "ARM:%lu AE:%lu D1:%lu",
           static_cast<unsigned long>(stats.rxArmCount),
           static_cast<unsigned long>(stats.rxArmErrors),
           static_cast<unsigned long>(stats.dio1PollHits));
  display.drawStr(0, 40, line);

  if (stats.rawRxCount > 0) {
    snprintf(line, sizeof(line), "RSSI:%4.0f SNR:%4.1f", stats.lastRssi, stats.lastSnr);
    display.drawStr(0, 50, line);
    snprintf(line, sizeof(line), "LAST:%s #%lu", lastSource.c_str(),
             static_cast<unsigned long>(stats.lastRxSequence));
  } else {
    snprintf(line, sizeof(line), "SELF:%lu ERR:%lu",
             static_cast<unsigned long>(stats.selfPacketCount),
             static_cast<unsigned long>(stats.crcOrRxErrors));
    display.drawStr(0, 50, line);
    snprintf(line, sizeof(line), "SF%u BW%.0f DIO1:%d", RF_SPREADING_FACTOR,
             RF_BANDWIDTH_KHZ, digitalRead(PIN_LORA_DIO1));
  }
  display.drawStr(0, 61, line);
  display.sendBuffer();
}

void scheduleNextTx(uint32_t baseDelayMs = TEST_TX_INTERVAL_MS) {
  // Large per-packet jitter makes repeated collisions between two AUTO nodes
  // extremely unlikely even if they boot at nearly the same time.
  const uint32_t jitter = esp_random() % (kTxJitterMs + 1);
  nextTxAt = millis() + baseDelayMs + jitter;
}

void startReceive(bool verbose = false) {
  receivedFlag = false;
  const int16_t state = radio.startReceive();
  stats.rxArmCount++;
  receivingArmed = state == RADIOLIB_ERR_NONE;

  if (!receivingArmed) {
    stats.rxArmErrors++;
    stats.crcOrRxErrors++;
  }

  if (verbose || !receivingArmed) {
    debugPrintf("[RXARM] #%lu state=%d DIO1=%d mode=%s\n",
                static_cast<unsigned long>(stats.rxArmCount), state,
                digitalRead(PIN_LORA_DIO1), labModeName());
  }
}

void printRfStatus() {
  const uint64_t mac = ESP.getEfuseMac();
  debugPrintf("\n[STATUS] node=%s mode=%s eFuse=%04X%08lX\n",
              nodeId.c_str(), labModeName(),
              static_cast<unsigned int>((mac >> 32) & 0xFFFFULL),
              static_cast<unsigned long>(mac & 0xFFFFFFFFULL));
  debugPrintf("[STATUS] TX=%lu RAW_RX=%lu RX=%lu SELF=%lu FOREIGN=%lu ERR=%lu\n",
              static_cast<unsigned long>(stats.txCount),
              static_cast<unsigned long>(stats.rawRxCount),
              static_cast<unsigned long>(stats.rxCount),
              static_cast<unsigned long>(stats.selfPacketCount),
              static_cast<unsigned long>(stats.foreignPacketCount),
              static_cast<unsigned long>(stats.crcOrRxErrors));
  debugPrintf("[STATUS] RX_ARM=%lu ARM_ERR=%lu armed=%s DIO1=%d poll_hits=%lu\n",
              static_cast<unsigned long>(stats.rxArmCount),
              static_cast<unsigned long>(stats.rxArmErrors),
              receivingArmed ? "YES" : "NO",
              digitalRead(PIN_LORA_DIO1),
              static_cast<unsigned long>(stats.dio1PollHits));
  if (stats.rawRxCount > 0) {
    debugPrintf("[STATUS] last=%s #%lu RSSI=%.1f SNR=%.1f\n",
                lastSource.c_str(), static_cast<unsigned long>(stats.lastRxSequence),
                stats.lastRssi, stats.lastSnr);
  }
}

void printLabHelp() {
  debugPrintf("\n[RF LAB] Runtime commands (single letter + Enter):\n");
  debugPrintf("  a = AUTO    periodic TX + continuous RX\n");
  debugPrintf("  t = TXONLY  transmit test packets, never listen\n");
  debugPrintf("  r = RXONLY  continuous receive, never transmit\n");
  debugPrintf("  s = STATUS  print detailed RF counters/state\n");
  debugPrintf("  h = HELP\n\n");
}

void setLabMode(LabMode mode) {
  labMode = mode;
  receivedFlag = false;

  if (labMode == LabMode::TxOnly) {
    receivingArmed = false;
    radio.clearDio1Action();
    radio.standby();
    scheduleNextTx(1000);
    debugPrintf("[MODE] TXONLY: RX disabled; first TX scheduled shortly.\n");
  } else {
    radio.setDio1Action(setFlag);
    startReceive(true);
    if (labMode == LabMode::Auto) {
      scheduleNextTx();
      debugPrintf("[MODE] AUTO: RX armed; randomized periodic TX enabled.\n");
    } else {
      nextTxAt = 0;
      debugPrintf("[MODE] RXONLY: continuous RX; TX completely disabled.\n");
    }
  }

  drawStatus();
}

void handleCommand(char c) {
  if (c == '\r' || c == '\n' || c == ' ' || c == '\t') return;

  switch (c) {
    case 'a': case 'A': setLabMode(LabMode::Auto); break;
    case 't': case 'T': setLabMode(LabMode::TxOnly); break;
    case 'r': case 'R': setLabMode(LabMode::RxOnly); break;
    case 's': case 'S': printRfStatus(); break;
    case 'h': case 'H': printLabHelp(); break;
    default:
      debugPrintf("[CMD] Unknown '%c'. Press h for help.\n", c);
      break;
  }
}

void pollSerialCommands() {
  while (Serial.available() > 0) {
    handleCommand(static_cast<char>(Serial.read()));
  }
  while (Serial0.available() > 0) {
    handleCommand(static_cast<char>(Serial0.read()));
  }
}

void transmitTestPacket() {
  if (labMode == LabMode::RxOnly) return;

  receivingArmed = false;
  receivedFlag = false;
  radio.clearDio1Action();

  String payload = makeTestPayload(nodeId, ++txSequence);
  debugPrintf("[TX] #%lu %s\n", static_cast<unsigned long>(txSequence), payload.c_str());

  const int16_t state = radio.transmit(payload);
  if (state == RADIOLIB_ERR_NONE) {
    stats.txCount++;
    debugPrintf("[TX] OK %.1f ms airtime\n", radio.getTimeOnAir(payload.length()) / 1000.0f);
  } else {
    debugPrintf("[TX] ERROR %d\n", state);
    stats.crcOrRxErrors++;
  }

  if (labMode == LabMode::Auto) {
    radio.setDio1Action(setFlag);
    startReceive(false);
  } else {
    radio.standby();
  }

  scheduleNextTx();
}

void handleReceive(bool fromDio1Poll = false) {
  receivedFlag = false;
  receivingArmed = false;

  if (fromDio1Poll) {
    stats.dio1PollHits++;
    debugPrintf("[RX] DIO1 polling fallback observed asserted IRQ.\n");
  }

  String payload;
  const int16_t state = radio.readData(payload);
  if (state != RADIOLIB_ERR_NONE) {
    debugPrintf("[RX] ERROR %d\n", state);
    stats.crcOrRxErrors++;
    if (labMode != LabMode::TxOnly) startReceive(false);
    return;
  }

  stats.rawRxCount++;
  stats.lastRssi = radio.getRSSI();
  stats.lastSnr = radio.getSNR();
  stats.lastRxAtMs = millis();

  String source;
  uint32_t sequence = 0;
  if (!parseTestPayload(payload, source, sequence)) {
    stats.foreignPacketCount++;
    lastSource = "FOREIGN";
    debugPrintf("[RX] RAW/FOREIGN RSSI %.1f SNR %.1f: %s\n",
                stats.lastRssi, stats.lastSnr, payload.c_str());
    if (labMode != LabMode::TxOnly) startReceive(false);
    return;
  }

  lastSource = source;
  stats.lastRxSequence = sequence;

  if (source == nodeId) {
    stats.selfPacketCount++;
    debugPrintf("[RX] SELF-ID packet %s #%lu RSSI %.1f SNR %.1f\n",
                source.c_str(), static_cast<unsigned long>(sequence),
                stats.lastRssi, stats.lastSnr);
    if (labMode != LabMode::TxOnly) startReceive(false);
    return;
  }

  stats.rxCount++;

  if (stats.haveLastRxSequence) {
    if (sequence == stats.lastRxSequence) {
      stats.duplicateCount++;
    } else if (sequence > stats.lastRxSequence + 1) {
      stats.missedSequenceEstimate += sequence - stats.lastRxSequence - 1;
    }
  }

  stats.lastRxSequence = sequence;
  stats.haveLastRxSequence = true;

  debugPrintf("[RX] %s #%lu RSSI %.1f dBm SNR %.1f dB RAW=%lu RX=%lu\n",
              source.c_str(), static_cast<unsigned long>(sequence),
              stats.lastRssi, stats.lastSnr,
              static_cast<unsigned long>(stats.rawRxCount),
              static_cast<unsigned long>(stats.rxCount));

  if (labMode != LabMode::TxOnly) startReceive(false);
}
#endif

void setup() {
  initDebugChannels();
  pulseBootStage(1);
  printBootDiagnostics();

  nodeId = makeNodeId();
  debugPrintf("[BOOT:1] application entered, node %s\n", nodeId.c_str());

#if YWD_RF_DIAGNOSTIC
  debugPrintf("[DIAG] Peripheral objects and initialization are disabled.\n");
  debugPrintf("[DIAG] Expect the status LED to toggle every 500 ms.\n");
  debugPrintf("[DIAG] If this works, the failure is later in OLED/SPI/SX1262 bring-up.\n");
  return;
#else
  pulseBootStage(2);
  debugPrintf("[BOOT:2] enabling Vext/OLED power on GPIO %d\n", PIN_VEXT);

  pinMode(PIN_VEXT, OUTPUT);
  digitalWrite(PIN_VEXT, LOW);
  delay(100);

  pulseBootStage(3);
  debugPrintf("[BOOT:3] resetting OLED on GPIO %d\n", PIN_OLED_RST);
  resetOled();

  debugPrintf("[BOOT:3] starting temporary I2C probe SDA=%d SCL=%d @ 100 kHz timeout=50 ms\n",
              PIN_OLED_SDA, PIN_OLED_SCL);
  Wire.setTimeOut(50);
  const bool wireOk = Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL, 100000);
  debugPrintf("[BOOT:3] probe Wire.begin returned %s\n", wireOk ? "true" : "false");

  bool oledPresent = false;
  if (wireOk) {
    oledPresent = oledAddressResponds();
    debugPrintf("[BOOT:3] ending temporary I2C probe before U8g2 ownership\n");
    Wire.end();
    delay(20);
  }

  if (oledPresent) {
    debugPrintf("[BOOT:3] OLED ACKed; U8g2 taking clean I2C ownership with reset=NONE\n");
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

  const int16_t switchState = radio.setDio2AsRfSwitch(true);
  debugPrintf("[BOOT:5] DIO2 RF switch control state=%d\n", switchState);
  if (switchState != RADIOLIB_ERR_NONE) {
    stats.crcOrRxErrors++;
  }

  radioOk = true;
  radio.setDio1Action(setFlag);
  startReceive(true);
  scheduleNextTx();

  const uint64_t mac = ESP.getEfuseMac();
  debugPrintf("[BOOT:6] SX1262 initialized successfully\n");
  debugPrintf("[BOOT:6] OLED status: %s\n", displayOk ? "ONLINE" : "HEADLESS");
  debugPrintf("[RF] node=%s eFuse=%04X%08lX\n",
              nodeId.c_str(),
              static_cast<unsigned int>((mac >> 32) & 0xFFFFULL),
              static_cast<unsigned long>(mac & 0xFFFFFFFFULL));
  debugPrintf("[RF] SX1262 OK %.3f MHz SF%u BW%.0f CR4/%u TX %d dBm TCXO %.1f V\n",
              RF_FREQUENCY_MHZ, RF_SPREADING_FACTOR, RF_BANDWIDTH_KHZ,
              RF_CODING_RATE, RF_TX_POWER_DBM, RF_TCXO_VOLTAGE);
  debugPrintf("[RF] AUTO TX spacing: %lu..%lu ms. RX is armed between packets.\n",
              static_cast<unsigned long>(TEST_TX_INTERVAL_MS),
              static_cast<unsigned long>(TEST_TX_INTERVAL_MS + kTxJitterMs));
  printLabHelp();

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

  pollSerialCommands();

  const bool dio1Polled = receivingArmed && digitalRead(PIN_LORA_DIO1) == HIGH;
  if (receivedFlag || dio1Polled) {
    handleReceive(dio1Polled && !receivedFlag);
  }

  const uint32_t now = millis();
  if (labMode != LabMode::RxOnly && nextTxAt != 0 &&
      static_cast<int32_t>(now - nextTxAt) >= 0) {
    transmitTestPacket();
  }

  if (now - lastDisplayAt >= DISPLAY_REFRESH_MS) {
    lastDisplayAt = now;
    drawStatus();
  }

  delay(5);
#endif
}
