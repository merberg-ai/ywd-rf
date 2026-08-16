#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "config.h"
#include "rf_lab.h"

#ifndef YWD_RF_VERSION
#define YWD_RF_VERSION "0.0.1-dev"
#endif

using namespace ywd;

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
String nodeId;
String lastSource = "-";
uint32_t txSequence = 0;
uint32_t nextTxAt = 0;
uint32_t lastDisplayAt = 0;
volatile bool receivedFlag = false;
bool radioOk = false;

void setFlag() { receivedFlag = true; }

void showBoot(const char* line1, const char* line2 = "") {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(0, 12, "YWD-RF RF LAB");
  display.drawStr(0, 28, line1);
  display.drawStr(0, 42, line2);
  display.sendBuffer();
}

void drawStatus() {
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
    Serial.printf("[RF] startReceive failed: %d\n", state);
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
  const String payload = makeTestPayload(nodeId, ++txSequence);
  Serial.printf("[TX] #%lu %s\n", static_cast<unsigned long>(txSequence), payload.c_str());

  radio.clearDio1Action();
  const int16_t state = radio.transmit(payload);
  if (state == RADIOLIB_ERR_NONE) {
    stats.txCount++;
    Serial.printf("[TX] OK  %.1f ms airtime\n", radio.getTimeOnAir(payload.length()) / 1000.0f);
  } else {
    Serial.printf("[TX] ERROR %d\n", state);
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
    Serial.printf("[RX] ERROR %d\n", state);
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
    Serial.printf("[RX] FOREIGN/UNKNOWN RSSI %.1f SNR %.1f: %s\n",
                  stats.lastRssi, stats.lastSnr, payload.c_str());
    startReceive();
    return;
  }

  if (source == nodeId) {
    Serial.printf("[RX] self packet ignored #%lu\n", static_cast<unsigned long>(sequence));
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

  Serial.printf("[RX] %s #%lu RSSI %.1f dBm SNR %.1f dB\n",
                source.c_str(), static_cast<unsigned long>(sequence),
                stats.lastRssi, stats.lastSnr);

  startReceive();
}

void setup() {
  Serial.begin(115200);
  delay(1200);

  nodeId = makeNodeId();

  // Heltec V3 Vext is active-low and powers the onboard OLED rail.
  pinMode(PIN_VEXT, OUTPUT);
  digitalWrite(PIN_VEXT, LOW);
  delay(20);

  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  display.setI2CAddress(OLED_ADDR << 1);
  display.begin();
  showBoot("Starting...", nodeId.c_str());

  Serial.println();
  Serial.println("================================================");
  Serial.printf(" YWD-RF %s - RF Lab\n", YWD_RF_VERSION);
  Serial.println("================================================");
  Serial.printf("Node: %s\n", nodeId.c_str());
  Serial.printf("CPU: %u MHz, Flash: %u bytes\n", ESP.getCpuFreqMHz(), ESP.getFlashChipSize());

  loraSpi.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_NSS);

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
    Serial.printf("[FATAL] SX1262 begin failed: %d\n", state);
    const String code = String(state);
    showBoot("SX1262 FAILED", code.c_str());
    while (true) delay(1000);
  }

  radioOk = true;
  radio.setDio1Action(setFlag);
  startReceive();
  scheduleNextTx();

  Serial.printf("[RF] SX1262 OK %.3f MHz SF%u BW%.0f CR4/%u TX %d dBm TCXO %.1f V\n",
                RF_FREQUENCY_MHZ, RF_SPREADING_FACTOR, RF_BANDWIDTH_KHZ,
                RF_CODING_RATE, RF_TX_POWER_DBM, RF_TCXO_VOLTAGE);
  Serial.println("[RF] Each node sends a numbered test packet about every 5 seconds.");
  Serial.println("[RF] Move the nodes apart and watch RSSI/SNR/missed counters.");

  drawStatus();
}

void loop() {
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
}
