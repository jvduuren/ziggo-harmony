#pragma once

#include <Arduino.h>

// HID report IDs used by the original remote (see docs/ble-profile.md).
enum : uint8_t {
  kReportKeypad = 1,    // Keyboard/Keypad usage page
  kReportSystem = 2,    // Generic Desktop, System Control
  kReportConsumer = 3,  // Consumer usage page
};

struct RemoteKey {
  const char* id;     // stable identifier: NVS key, API and serial console
  const char* label;  // shown in the web UI
  uint8_t report;
  uint16_t usage;
};

// A decoded IR code as learned from the user's remote. bits == 0 means "not set".
struct IrCode {
  int16_t protocol;  // IRremoteESP8266 decode_type_t
  uint16_t bits;
  uint64_t value;

  bool valid() const { return bits != 0; }
  bool operator==(const IrCode& o) const {
    return protocol == o.protocol && bits == o.bits && value == o.value;
  }
};

extern const RemoteKey kKeys[];
extern const size_t kKeyCount;

namespace keymap {

void load();
int find(const char* id);            // key index, or -1
int lookup(const IrCode& code);      // key index mapped to this IR code, or -1
const IrCode& code(int index);
void assign(int index, const IrCode& code);  // also removes the code from other keys
void clear(int index);
void clearAll();

// Plain-text mapping, one line per key: "<key id> <protocol> <bits> <hex value>".
// Meant for sharing a mapping for a given remote/Harmony device profile.
String exportText();
String importText(const String& text);  // returns a human-readable result

}  // namespace keymap
