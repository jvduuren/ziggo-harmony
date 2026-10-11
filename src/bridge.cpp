#include "bridge.h"

#include "ble_remote.h"
#include "config.h"
#include "keys.h"
#include "led.h"
#include "log.h"

namespace {

int heldKey = -1;
uint32_t heldSince = 0;
uint32_t lastFrameAt = 0;

int learning = -1;
uint32_t learningSince = 0;
uint32_t ignoreIrUntil = 0;  // swallow the rest of the burst that was just learned

IrCode last{};
int lastKey = -1;
uint32_t lastAt = 0;

void pressKey(int index) {
  ble::press(kKeys[index].report, kKeys[index].usage);
  heldKey = index;
  heldSince = millis();
  logf("[key] %s down%s\n", kKeys[index].id, ble::ready() ? "" : " (box not connected)");
}

void releaseKey() {
  if (heldKey < 0) return;
  ble::release(kKeys[heldKey].report);
  logf("[key] %s up (%lu ms)\n", kKeys[heldKey].id, millis() - heldSince);
  heldKey = -1;
}

}  // namespace

namespace bridge {

void handleIr(const IrEvent& event) {
  uint32_t now = millis();
  if ((int32_t)(ignoreIrUntil - now) > 0) return;

  if (event.repeat) {
    // "Still held" frame without a code of its own (NEC style).
    if (heldKey >= 0) lastFrameAt = now;
    return;
  }

  last = event.code;
  lastAt = now;

  if (learning >= 0) {
    // Keep waiting for a real code: a stray UNKNOWN frame often precedes the button's code,
    // and learning it would also swallow the real code right after it.
    if (!ir::decoded(event.code)) {
      logf("[learn] skipped %s, waiting for a decodable code\n", ir::describe(event.code).c_str());
      return;
    }
    keymap::add(learning, event.code);
    logf("[learn] %s = %s\n", kKeys[learning].id, ir::describe(event.code).c_str());
    led::flash(0, 30, 0, 400);
    learning = -1;
    lastKey = keymap::lookup(event.code);
    ignoreIrUntil = now + 600;
    return;
  }

  int key = keymap::lookup(event.code);
  lastKey = key;
  if (key < 0) {
    logf("[ir] unknown %s\n", ir::describe(event.code).c_str());
    led::flash(30, 0, 0, 80);
    return;
  }

  if (key == heldKey && !event.toggled) {
    lastFrameAt = now;  // same button still held
    return;
  }
  releaseKey();
  pressKey(key);
  lastFrameAt = now;
  led::flash(24, 24, 24, 60);
}

void tick() {
  uint32_t now = millis();
  if (heldKey >= 0 && now - lastFrameAt > kReleaseTimeoutMs) releaseKey();
  if (learning >= 0 && now - learningSince > kLearnTimeoutMs) {
    logf("[learn] timed out\n");
    learning = -1;
  }
}

void tap(int keyIndex) {
  releaseKey();
  pressKey(keyIndex);
  delay(kTapDurationMs);
  releaseKey();
}

void tapRaw(uint8_t report, uint16_t usage) {
  releaseKey();
  logf("[key] raw report %u usage 0x%04X\n", report, usage);
  ble::press(report, usage);
  delay(kTapDurationMs);
  ble::release(report);
}

bool startLearning(int keyIndex) {
  if (keyIndex < 0 || keyIndex >= (int)kKeyCount) return false;
  releaseKey();
  learning = keyIndex;
  learningSince = millis();
  logf("[learn] press the button for '%s' on your remote\n", kKeys[keyIndex].label);
  return true;
}

void cancelLearning() { learning = -1; }

int learningKey() { return learning; }

const IrCode& lastCode() { return last; }

int lastCodeKey() { return lastKey; }

uint32_t lastCodeAgeMs() { return lastAt ? millis() - lastAt : UINT32_MAX; }

}  // namespace bridge
