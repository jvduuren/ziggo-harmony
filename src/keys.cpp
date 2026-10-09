#include "keys.h"

#include <IRutils.h>
#include <Preferences.h>

// Every button of the original remote that reaches the box over BLE, with the exact
// report and usage it sends. TV power, volume, mute and source are IR-only (they go
// straight to the TV) and are therefore not listed.
const RemoteKey kKeys[] = {
    {"power", "Standby", kReportConsumer, 0x003F},
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

IrCode codes[sizeof(kKeys) / sizeof(kKeys[0])];
Preferences prefs;

}  // namespace

namespace keymap {

void load() {
  prefs.begin("irmap", false);
  for (size_t i = 0; i < kKeyCount; i++) {
    codes[i] = {};
    if (prefs.isKey(kKeys[i].id) && prefs.getBytesLength(kKeys[i].id) == sizeof(IrCode)) {
      prefs.getBytes(kKeys[i].id, &codes[i], sizeof(IrCode));
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
    if (codes[i] == code) return i;
  }
  return -1;
}

const IrCode& code(int index) { return codes[index]; }

void assign(int index, const IrCode& code) {
  for (size_t i = 0; i < kKeyCount; i++) {
    if ((int)i != index && codes[i] == code) clear(i);
  }
  codes[index] = code;
  prefs.putBytes(kKeys[index].id, &codes[index], sizeof(IrCode));
}

void clear(int index) {
  codes[index] = {};
  prefs.remove(kKeys[index].id);
}

void clearAll() {
  for (size_t i = 0; i < kKeyCount; i++) codes[i] = {};
  prefs.clear();
}

String exportText() {
  String out = "# ziggo-harmony IR mapping v1: <key> <protocol> <bits> <hex value>\n";
  for (size_t i = 0; i < kKeyCount; i++) {
    if (!codes[i].valid()) continue;
    out += String(kKeys[i].id) + " " + typeToString((decode_type_t)codes[i].protocol) + " " +
           codes[i].bits + " 0x" + uint64ToString(codes[i].value, 16) + "\n";
  }
  return out;
}

String importText(const String& text) {
  int imported = 0;
  String errors;
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
    assign(key, IrCode{(int16_t)type, (uint16_t)bits, strtoull(value, nullptr, 0)});
    imported++;
  }
  return String("Imported ") + imported + " key(s).\n" + errors;
}

}  // namespace keymap
