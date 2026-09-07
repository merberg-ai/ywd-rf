#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <esp_system.h>
#include <stdarg.h>

#include "config.h"

#ifndef YWD_RF_VERSION
#define YWD_RF_VERSION "0.0.1-dev"
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
bool oledFound = false;
bool oledInitialized = false;
bool radioAttempted = false;
int16_t radioState = RADIOLIB_ERR_UNKNOWN;

void debugPrintf(const char* format, ...) {
  char buffer[384];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  Serial.print(buffer);
  Serial0.print(buffer);
}

void setStatusLed(bool on) {
  digitalWrite(kStatusLedPin, on ? HIGH : LOW);
}

void updateHeartbeat() {
  const uint32_t now = millis();
  if (now - lastHeartbeatAt >= kHeartbeatIntervalMs) {
    lastHeartbeatAt = now;
    heartbeatState = !heartbeatState;
    setStatusLed(heartbeatState);
  }
}

void initDebug() {
  pinMode(kStatusLedPin, OUTPUT);
  setStatusLed(true);
  delay(180);
  setStatusLed(false);
  delay(80);

  Serial0.begin(kDebugBaud);
  Serial.begin(kDebugBaud);
  Serial.setDebugOutput(true);
  delay(100);
}

void printHeader() {
  debugPrintf("\n================================================\n");
  debugPrintf(" YWD-RF %s - PERIPHERAL PROBE\n", YWD_RF_VERSION);
  debugPrintf("================================================\n");
  debugPrintf("[PROBE] reset reason: %d\n", static_cast<int>(esp_reset_reason()));
  debugPrintf("[PROBE] CPU: %u MHz  flash: %u  heap: %u\n",
              ESP.getCpuFreqMHz(), ESP.getFlashChipSize(), ESP.getFreeHeap());
  debugPrintf("[PROBE] OLED pins: VEXT=%d SDA=%d SCL=%d RST=%d addr=0x%02X\n",
              PIN_VEXT, PIN_OLED_SDA, PIN_OLED_SCL, PIN_OLED_RST, OLED_ADDR);
  debugPrintf("[PROBE] LoRa pins: NSS=%d DIO1=%d RST=%d BUSY=%d SCK=%d MISO=%d MOSI=%d\n",
              PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RST, PIN_LORA_BUSY,
              PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI);
}

void probeI2cAndOled() {
  debugPrintf("\n[OLED:1] Sampling I2C pins before Vext...\n");
  pinMode(PIN_OLED_SDA, INPUT_PULLUP);
  pinMode(PIN_OLED_SCL, INPUT_PULLUP);
  delay(20);
  debugPrintf("[OLED:1] SDA=%d SCL=%d\n", digitalRead(PIN_OLED_SDA), digitalRead(PIN_OLED_SCL));

  debugPrintf("[OLED:2] Enabling Vext: GPIO %d -> LOW\n", PIN_VEXT);
  pinMode(PIN_VEXT, OUTPUT);
  digitalWrite(PIN_VEXT, LOW);
  delay(100);
  debugPrintf("[OLED:2] after Vext: SDA=%d SCL=%d\n",
              digitalRead(PIN_OLED_SDA), digitalRead(PIN_OLED_SCL));

  debugPrintf("[OLED:3] Starting Wire on SDA=%d SCL=%d @ 100 kHz, timeout 50 ms...\n",
              PIN_OLED_SDA, PIN_OLED_SCL);
  Wire.setTimeOut(50);
  const bool wireOk = Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL, 100000);
  debugPrintf("[OLED:3] Wire.begin returned %s\n", wireOk ? "true" : "false");
  if (!wireOk) {
    debugPrintf("[OLED:STOP] I2C controller did not initialize; skipping OLED library.\n");
    return;
  }

  debugPrintf("[OLED:4] Scanning I2C addresses 0x08..0x77...\n");
  uint8_t foundCount = 0;
  for (uint8_t address = 0x08; address <= 0x77; ++address) {
    Wire.beginTransmission(address);
    const uint8_t result = Wire.endTransmission(true);
    if (result == 0) {
      ++foundCount;
      debugPrintf("[OLED:4] ACK at 0x%02X%s\n",
                  address, address == OLED_ADDR ? "  <--- expected OLED" : "");
      if (address == OLED_ADDR) oledFound = true;
    } else if (result == 5) {
      debugPrintf("[OLED:4] timeout while probing 0x%02X\n", address);
    }
    delay(1);
  }
  debugPrintf("[OLED:4] scan complete: %u device(s), expected OLED %s\n",
              foundCount, oledFound ? "FOUND" : "NOT FOUND");

  if (!oledFound) {
    debugPrintf("[OLED:STOP] Nothing ACKed at 0x%02X; not calling U8g2 begin().\n", OLED_ADDR);
    return;
  }

  debugPrintf("[OLED:5] Calling U8g2 SSD1306 begin() now. If output stops here, this call is the hang.\n");
  static U8G2_SSD1306_128X64_NONAME_F_HW_I2C probeDisplay(
      U8G2_R0, PIN_OLED_RST, PIN_OLED_SCL, PIN_OLED_SDA);
  probeDisplay.setI2CAddress(OLED_ADDR << 1);
  probeDisplay.begin();
  oledInitialized = true;
  debugPrintf("[OLED:5] U8g2 begin returned successfully.\n");

  probeDisplay.clearBuffer();
  probeDisplay.setFont(u8g2_font_6x10_tf);
  probeDisplay.drawStr(0, 12, "YWD-RF PROBE");
  probeDisplay.drawStr(0, 30, "OLED: OK");
  probeDisplay.drawStr(0, 48, "Serial: check log");
  probeDisplay.sendBuffer();
  debugPrintf("[OLED:6] Test frame sent to display.\n");
}

bool radioBusyStuckHigh() {
  pinMode(PIN_LORA_BUSY, INPUT);
  pinMode(PIN_LORA_DIO1, INPUT);

  uint8_t busyHighSamples = 0;
  for (uint8_t i = 0; i < 10; ++i) {
    const int busy = digitalRead(PIN_LORA_BUSY);
    const int dio1 = digitalRead(PIN_LORA_DIO1);
    debugPrintf("[LORA:1] sample %u BUSY=%d DIO1=%d\n", i + 1, busy, dio1);
    if (busy) ++busyHighSamples;
    delay(20);
  }
  return busyHighSamples == 10;
}

void probeRadio() {
  debugPrintf("\n[LORA:1] Sampling SX1262 control pins before SPI...\n");
  if (radioBusyStuckHigh()) {
    debugPrintf("[LORA:STOP] BUSY stayed HIGH for all samples. Skipping RadioLib begin to avoid a possible hang.\n");
    debugPrintf("[LORA:STOP] This strongly suggests a wrong BUSY pin, missing radio power, or incompatible board wiring.\n");
    return;
  }

  debugPrintf("[LORA:2] BUSY can go LOW. Starting FSPI on SCK=%d MISO=%d MOSI=%d NSS=%d...\n",
              PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_NSS);
  static SPIClass probeSpi(FSPI);
  static SPISettings probeSpiSettings(2000000, MSBFIRST, SPI_MODE0);
  probeSpi.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_NSS);
  debugPrintf("[LORA:2] SPI begin returned.\n");

  debugPrintf("[LORA:3] Constructing SX1262 and calling RadioLib begin(). No TX will be performed.\n");
  static SX1262 probeRadio = new Module(
      PIN_LORA_NSS,
      PIN_LORA_DIO1,
      PIN_LORA_RST,
      PIN_LORA_BUSY,
      probeSpi,
      probeSpiSettings);

  probeRadio.tcxoVoltage = RF_TCXO_VOLTAGE;
  radioAttempted = true;
  radioState = probeRadio.begin(
      RF_FREQUENCY_MHZ,
      RF_BANDWIDTH_KHZ,
      RF_SPREADING_FACTOR,
      RF_CODING_RATE,
      RF_SYNC_WORD,
      RF_TX_POWER_DBM,
      RF_PREAMBLE_LEN,
      RF_TCXO_VOLTAGE,
      false);

  debugPrintf("[LORA:3] RadioLib begin returned state=%d (%s)\n",
              radioState,
              radioState == RADIOLIB_ERR_NONE ? "SUCCESS" : "FAILURE");
  debugPrintf("[LORA:3] BUSY=%d DIO1=%d\n",
              digitalRead(PIN_LORA_BUSY), digitalRead(PIN_LORA_DIO1));

  if (radioState == RADIOLIB_ERR_NONE) {
    probeRadio.standby();
    debugPrintf("[LORA:4] SX1262 initialized and placed in standby. RF transmit was NOT enabled.\n");
  }
}
}  // namespace

void setup() {
  initDebug();
  printHeader();

  debugPrintf("\n[PROBE] Stage A: Vext / I2C / OLED\n");
  probeI2cAndOled();

  debugPrintf("\n[PROBE] Stage B: SX1262 control pins / SPI / RadioLib init\n");
  probeRadio();

  debugPrintf("\n================================================\n");
  debugPrintf(" PERIPHERAL PROBE COMPLETE\n");
  debugPrintf(" OLED address 0x%02X: %s\n", OLED_ADDR, oledFound ? "FOUND" : "NOT FOUND");
  debugPrintf(" OLED U8g2 init: %s\n", oledInitialized ? "RETURNED" : "NOT RUN / DID NOT RETURN");
  if (radioAttempted) {
    debugPrintf(" SX1262 RadioLib state: %d\n", radioState);
  } else {
    debugPrintf(" SX1262 RadioLib state: NOT ATTEMPTED\n");
  }
  debugPrintf("================================================\n");
}

void loop() {
  updateHeartbeat();

  static uint32_t lastReportAt = 0;
  const uint32_t now = millis();
  if (now - lastReportAt >= 3000) {
    lastReportAt = now;
    debugPrintf("[PROBE] alive uptime=%lu heap=%u SDA=%d SCL=%d BUSY=%d DIO1=%d\n",
                static_cast<unsigned long>(now), ESP.getFreeHeap(),
                digitalRead(PIN_OLED_SDA), digitalRead(PIN_OLED_SCL),
                digitalRead(PIN_LORA_BUSY), digitalRead(PIN_LORA_DIO1));
  }
  delay(5);
}
