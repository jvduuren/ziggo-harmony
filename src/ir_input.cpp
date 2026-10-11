#include "ir_input.h"

#include <IRrecv.h>
#include <IRutils.h>

#include "config.h"
#include "log.h"

namespace {

constexpr uint16_t kCaptureBufferSize = 1024;
constexpr uint8_t kFrameTimeoutMs = 15;

IRrecv receiver(IR_PIN, kCaptureBufferSize, kFrameTimeoutMs, true);
decode_results results;
uint64_t lastToggleBit = 0;

// RC5 and RC6 flip a toggle bit on every new button press. Strip it so a button maps to
// one code, but remember it so a quick second press is not mistaken for a held button.
uint64_t toggleMask(const IrCode& code) {
  switch (code.protocol) {
    case decode_type_t::RC5:
    case decode_type_t::RC5X:
      return 1ULL << 11;
    case decode_type_t::RC6:
      return code.bits == 36 ? 1ULL << 15 : 1ULL << 16;
    default:
      return 0;
  }
}

}  // namespace

namespace ir {

void begin() {
  receiver.setUnknownThreshold(12);
  receiver.enableIRIn();
  logf("[ir] receiver on GPIO%d\n", IR_PIN);
}

bool poll(IrEvent& event) {
  if (!receiver.decode(&results)) return false;
  event.code.protocol = results.decode_type;
  event.code.bits = results.bits;
  event.code.value = results.value;
  event.repeat = results.repeat;
  event.toggled = false;
  uint64_t mask = toggleMask(event.code);
  if (mask) {
    uint64_t toggleBit = event.code.value & mask;
    event.toggled = toggleBit != lastToggleBit;
    lastToggleBit = toggleBit;
    event.code.value &= ~mask;
  }
  return true;
}

String describe(const IrCode& code) {
  if (!code.valid()) return String();
  return typeToString((decode_type_t)code.protocol) + " " + code.bits + " bit 0x" +
         uint64ToString(code.value, 16);
}

bool decoded(const IrCode& code) { return code.protocol != decode_type_t::UNKNOWN; }

}  // namespace ir
