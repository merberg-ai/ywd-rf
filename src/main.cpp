#include <Arduino.h>

namespace {
constexpr char kProjectName[] = "YWD-RF";
constexpr char kVersion[] = "0.0.1-dev";
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("========================================");
  Serial.printf("%s %s\n", kProjectName, kVersion);
  Serial.println("ESP32-S3 project bootstrap");
  Serial.println("RF Lab bring-up will be developed on dev.");
  Serial.println("========================================");
}

void loop() {
  delay(1000);
}
