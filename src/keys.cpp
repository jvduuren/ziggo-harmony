#include "keys.h"

#include <IRutils.h>
#include <Preferences.h>

#include "ir_input.h"

// Every button of the original remote that reaches the box over BLE, with the exact
// report and usage it sends. TV power, volume, mute and source are IR-only (they go
// straight to the TV) and are therefore not listed.
const RemoteKey kKeys[] = {
    // The original remote sends 0x003F, which the box acknowledges but ignores when it comes
    // from us; Consumer Power (0x0030) toggles standby.
    {"power", "Standby", kReportConsumer, 0x0030},
    {"home", "Home", kReportConsumer, 0x0223},
    {"back", "Back", kReportConsumer, 0x0224},
    {"guide", "Guide", kReportConsumer, 0x008D},
    {"tv", "TV", kReportConsumer, 0x0089},
    {"up", "Up", kReportConsumer, 0x0042},
    {"down", "Down", kReportConsumer, 0x0043},
    {"left", "Left", kReportConsumer, 0x0044},
    {"right", "Right", kReportConsumer, 0x0045},
    {"ok", "OK", kReportConsumer, 0x0041},
    {"ch_up", "Channel +", kReportConsumer, 0x009C},
    {"ch_down", "Channel -", kReportConsumer, 0x009D},
    {"play_pause", "Play/Pause", kReportConsumer, 0x00CD},
    {"rewind", "Rewind", kReportConsumer, 0x00B4},
    {"forward", "Fast forward", kReportConsumer, 0x00B3},
    {"record", "Record", kReportConsumer, 0x00B2},
    {"search", "Search", kReportConsumer, 0x0221},
    {"profile", "Profile", kReportConsumer, 0x018D},
    {"1", "1", kReportKeypad, 0x001E},
    {"2", "2", kReportKeypad, 0x001F},
    {"3", "3", kReportKeypad, 0x0020},
    {"4", "4", kReportKeypad, 0x0021},
    {"5", "5", kReportKeypad, 0x0022},
    {"6", "6", kReportKeypad, 0x0023},
    {"7", "7", kReportKeypad, 0x0024},
    {"8", "8", kReportKeypad, 0x0025},
    {"9", "9", kReportKeypad, 0x0026},
    {"0", "0", kReportKeypad, 0x0027},
};
const size_t kKeyCount = sizeof(kKeys) / sizeof(kKeys[0]);

namespace {

// Per key the valid codes come first; the rest is zeroed. Stored per key as a blob of
// 1..kMaxCodesPerKey IrCodes, so a mapping saved by older firmware (one code) still loads.
IrCode codes[sizeof(kKeys) / sizeof(kKeys[0])][kMaxCodesPerKey];
Preferences prefs;

void save(int index) {
  size_t n = keymap::codeCount(index);
  if (n) {
    prefs.putBytes(kKeys[index].id, codes[index], n * sizeof(IrCode));
  } else {
    prefs.remove(kKeys[index].id);
  }
}

// Removes the code from the key without saving; true if it was there.
bool removeCode(int index, const IrCode& code) {
  IrCode* slots = codes[index];
  for (size_t s = 0; s < kMaxCodesPerKey; s++) {
    if (!slots[s].valid() || !(slots[s] == code)) continue;
    for (size_t t = s; t + 1 < kMaxCodesPerKey; t++) slots[t] = slots[t + 1];
    slots[kMaxCodesPerKey - 1] = {};
    return true;
  }
  return false;
}

}  // namespace

namespace keymap {

void load() {
  prefs.begin("irmap", false);
  for (size_t i = 0; i < kKeyCount; i++) {
    memset(codes[i], 0, sizeof(codes[i]));
    if (!prefs.isKey(kKeys[i].id)) continue;
    size_t len = prefs.getBytesLength(kKeys[i].id);
    if (len && len % sizeof(IrCode) == 0 && len <= sizeof(codes[i])) {
      prefs.getBytes(kKeys[i].id, codes[i], len);
    }
  }
}

int find(const char* id) {
  for (size_t i = 0; i < kKeyCount; i++) {
    if (strcmp(kKeys[i].id, id) == 0) return i;
  }
  return -1;
}

int lookup(const IrCode& code) {
  if (!code.valid()) return -1;
  for (size_t i = 0; i < kKeyCount; i++) {
    for (const IrCode& c : codes[i]) {
      if (c.valid() && c == code) return i;
    }
  }
  return -1;
}

size_t codeCount(int index) {
  size_t n = 0;
  while (n < kMaxCodesPerKey && codes[index][n].valid()) n++;
  return n;
}

const IrCode& code(int index, size_t slot) { return codes[index][slot]; }

String describe(int index) {
  String out;
  for (size_t s = 0; s < codeCount(index); s++) {
    if (s) out += "\n";
    out += ir::describe(codes[index][s]);
  }
  return out;
}

void add(int index, const IrCode& code) {
  for (size_t i = 0; i < kKeyCount; i++) {
    if (removeCode(i, code) && (int)i != index) save(i);
  }
  IrCode* slots = codes[index];
  size_t n = codeCount(index);
  if (n == kMaxCodesPerKey) {
    for (size_t t = 0; t + 1 < kMaxCodesPerKey; t++) slots[t] = slots[t + 1];
    n--;
  }
  slots[n] = code;
  save(index);
}

void clear(int index) {
  memset(codes[index], 0, sizeof(codes[index]));
  prefs.remove(kKeys[index].id);
}

void clearAll() {
  memset(codes, 0, sizeof(codes));
  prefs.clear();
}

String exportText() {
  String out = "# ziggo-harmony IR mapping v1: <key> <protocol> <bits> <hex value>\n";
  for (size_t i = 0; i < kKeyCount; i++) {
    for (size_t s = 0; s < codeCount(i); s++) {
      const IrCode& c = codes[i][s];
      out += String(kKeys[i].id) + " " + typeToString((decode_type_t)c.protocol) + " " + c.bits +
             " 0x" + uint64ToString(c.value, 16) + "\n";
    }
  }
  return out;
}

String importText(const String& text) {
  int imported = 0;
  String errors;
  bool replaced[sizeof(kKeys) / sizeof(kKeys[0])] = {};  // a key in the import loses its old codes
  int start = 0;
  while (start < (int)text.length()) {
    int end = text.indexOf('\n', start);
    if (end < 0) end = text.length();
    String line = text.substring(start, end);
    start = end + 1;
    line.trim();
    if (!line.length() || line[0] == '#') continue;

    char id[24], proto[32], value[24];
    unsigned bits = 0;
    int key = -1;
    decode_type_t type = decode_type_t::UNKNOWN;
    if (sscanf(line.c_str(), "%23s %31s %u %23s", id, proto, &bits, value) == 4) {
      key = find(id);
      type = strToDecodeType(proto);
    }
    if (key < 0 || bits == 0 || (type == decode_type_t::UNKNOWN && strcmp(proto, "UNKNOWN") != 0)) {
      errors += "  skipped: " + line + "\n";
      continue;
    }
    if (!replaced[key]) {
      clear(key);
      replaced[key] = true;
    }
    add(key, IrCode{(int16_t)type, (uint16_t)bits, strtoull(value, nullptr, 0)});
    imported++;
  }
  return String("Imported ") + imported + " key(s).\n" + errors;
}

}  // namespace keymap
