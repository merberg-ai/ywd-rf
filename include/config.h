#pragma once

#include <Arduino.h>

namespace ywd {

// Heltec WiFi LoRa 32 V3 compatible pinout used by Meshtastic/Heltec V3 targets.
// If a clone differs, these are intentionally centralized for easy correction.
constexpr int PIN_LORA_NSS   = 8;
constexpr int PIN_LORA_DIO1  = 14;
constexpr int PIN_LORA_RST   = 12;
constexpr int PIN_LORA_BUSY  = 13;
constexpr int PIN_LORA_SCK   = 9;
constexpr int PIN_LORA_MISO  = 11;
constexpr int PIN_LORA_MOSI  = 10;

constexpr int PIN_OLED_SDA   = 17;
constexpr int PIN_OLED_SCL   = 18;
constexpr int PIN_OLED_RST   = 21;
constexpr int PIN_VEXT       = 36;  // active-low external/OLED power on Heltec V3
constexpr uint8_t OLED_ADDR  = 0x3C;

// Heltec V3 SX1262 uses a TCXO controlled from DIO3 at 1.8 V.
constexpr float RF_TCXO_VOLTAGE = 1.8;

// RF Lab defaults. Kept away from the local Meshtastic MediumFast slot so this
// firmware does not accidentally participate in that mesh.
constexpr float RF_FREQUENCY_MHZ = 915.500;
constexpr float RF_BANDWIDTH_KHZ = 125.0;
constexpr uint8_t RF_SPREADING_FACTOR = 7;
constexpr uint8_t RF_CODING_RATE = 5;
constexpr uint8_t RF_SYNC_WORD = 0x12; // RadioLib private LoRa sync word
constexpr int8_t RF_TX_POWER_DBM = 10;
constexpr uint16_t RF_PREAMBLE_LEN = 8;

constexpr uint32_t TEST_TX_INTERVAL_MS = 5000;
constexpr uint32_t DISPLAY_REFRESH_MS = 500;

}  // namespace ywd
