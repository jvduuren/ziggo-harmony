#pragma once

#include <Arduino.h>

#include "keys.h"

struct IrEvent {
  IrCode code;   // normalized: RC5/RC6 toggle bit cleared
  bool repeat;   // protocol repeat frame (e.g. NEC "button still held")
  bool toggled;  // RC5/RC6 toggle bit differs from the previous frame: a new press
};

namespace ir {

void begin();
bool poll(IrEvent& event);
String describe(const IrCode& code);  // e.g. "NEC 32 bit 0x20DF10EF"
// False for UNKNOWN: a hash of raw timings, which differs on every press for protocols the
// library cannot decode, and which is also what stray noise and partial frames decode to.
bool decoded(const IrCode& code);

}  // namespace ir
