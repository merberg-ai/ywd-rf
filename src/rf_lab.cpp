#include "rf_lab.h"

#include <esp_system.h>

namespace ywd {

String makeNodeId() {
  const uint64_t mac = ESP.getEfuseMac();
  char out[16];
  snprintf(out, sizeof(out), "YWD-%04X", static_cast<uint16_t>(mac & 0xFFFF));
  return String(out);
}

String makeTestPayload(const String& nodeId, uint32_t sequence) {
  char out[96];
  snprintf(out, sizeof(out), "YRF1|TEST|%s|%lu", nodeId.c_str(), static_cast<unsigned long>(sequence));
  return String(out);
}

bool parseTestPayload(const String& payload, String& sourceNode, uint32_t& sequence) {
  if (!payload.startsWith("YRF1|TEST|")) return false;

  const int nodeStart = 10;
  const int sep = payload.indexOf('|', nodeStart);
  if (sep < 0) return false;

  sourceNode = payload.substring(nodeStart, sep);
  const String seqText = payload.substring(sep + 1);
  if (seqText.length() == 0) return false;

  char* end = nullptr;
  sequence = strtoul(seqText.c_str(), &end, 10);
  return end && *end == '\0';
}

}  // namespace ywd
