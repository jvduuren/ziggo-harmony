#pragma once

#include <Arduino.h>

// Log to USB serial and keep the most recent lines for the web UI (/api/log), so the bridge
// can be followed without a USB connection.
void logf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
String logText();
