#pragma once

#include <Arduino.h>

#define FW_VERSION "0.2.0"

// GPIO of the 38 kHz IR receiver output (VS1838B / TSOP38238). Set per board in platformio.ini.
#ifndef IR_PIN
#define IR_PIN 1
#endif

// BOOT button, used for "hold to unpair" / "hold to reset Wi-Fi".
#ifndef BOOT_BUTTON_PIN
#define BOOT_BUTTON_PIN 0
#endif

// Name the box looks for; copied from the original remote.
constexpr const char* kBleName = "RemoteUnit";

// Release a held key when no IR frame arrived for this long. Harmony repeats a held
// button every ~45-115 ms depending on the protocol.
constexpr uint32_t kReleaseTimeoutMs = 200;
// Duration of a key press sent from the web UI, API or serial console.
constexpr uint32_t kTapDurationMs = 80;
constexpr uint32_t kLearnTimeoutMs = 20000;

constexpr uint32_t kUnpairHoldMs = 5000;
constexpr uint32_t kWifiResetHoldMs = 10000;

constexpr const char* kHostname = "ziggo-remote";
constexpr const char* kSetupApName = "ZiggoRemote-Setup";
