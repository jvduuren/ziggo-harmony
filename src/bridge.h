#pragma once

#include <Arduino.h>

#include "ir_input.h"

// Turns IR events into BLE key presses, handles held buttons and learning.
namespace bridge {

void handleIr(const IrEvent& event);
void tick();

void tap(int keyIndex);
void tapRaw(uint8_t report, uint16_t usage);  // for discovering extra usages

bool startLearning(int keyIndex);
void cancelLearning();
int learningKey();  // -1 when not learning

const IrCode& lastCode();
int lastCodeKey();  // key index the last IR code is mapped to, or -1
uint32_t lastCodeAgeMs();

}  // namespace bridge
