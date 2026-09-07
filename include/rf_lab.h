#pragma once

#include <Arduino.h>

namespace ywd {

struct RfStats {
  uint32_t txCount = 0;
  uint32_t rawRxCount = 0;
  uint32_t rxCount = 0;
  uint32_t crcOrRxErrors = 0;
  uint32_t duplicateCount = 0;
  uint32_t missedSequenceEstimate = 0;
  uint32_t selfPacketCount = 0;
  uint32_t foreignPacketCount = 0;
  uint32_t rxArmCount = 0;
  uint32_t rxArmErrors = 0;
  uint32_t dio1PollHits = 0;
  uint32_t lastRxSequence = 0;
  bool haveLastRxSequence = false;
  float lastRssi = 0.0f;
  float lastSnr = 0.0f;
  uint32_t lastRxAtMs = 0;
};

String makeNodeId();
String makeTestPayload(const String& nodeId, uint32_t sequence);
bool parseTestPayload(const String& payload, String& sourceNode, uint32_t& sequence);

}  // namespace ywd
