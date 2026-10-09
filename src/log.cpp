#include "log.h"

#include <stdarg.h>

#include <mutex>

namespace {

constexpr size_t kRingSize = 6144;
char ring[kRingSize];
size_t head = 0;
bool wrapped = false;
std::mutex ringMutex;  // logf is called from the Arduino loop and from NimBLE callbacks

}  // namespace

void logf(const char* fmt, ...) {
  char line[224];
  uint32_t ms = millis();
  int prefix = snprintf(line, sizeof(line), "%6lu.%03lu ", ms / 1000, ms % 1000);
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(line + prefix, sizeof(line) - prefix, fmt, args);
  va_end(args);
  if (n < 0) return;
  size_t len = min(sizeof(line) - 1, (size_t)(prefix + n));

  Serial.write(reinterpret_cast<const uint8_t*>(line), len);
  std::lock_guard<std::mutex> lock(ringMutex);
  for (size_t i = 0; i < len; i++) {
    ring[head] = line[i];
    head = (head + 1) % kRingSize;
    if (head == 0) wrapped = true;
  }
}

String logText() {
  std::lock_guard<std::mutex> lock(ringMutex);
  String out;
  if (!wrapped) {
    out.concat(ring, head);
    return out;
  }
  out.reserve(kRingSize);
  out.concat(ring + head, kRingSize - head);
  out.concat(ring, head);
  int firstNewline = out.indexOf('\n');  // drop the partially overwritten first line
  return firstNewline >= 0 ? out.substring(firstNewline + 1) : out;
}
