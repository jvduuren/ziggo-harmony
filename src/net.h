#pragma once

#include <Arduino.h>

// Wi-Fi (setup portal on first start), web UI, HTTP API and firmware upload.
// Everything here is optional: the IR to BLE bridge works without Wi-Fi.
namespace net {

void begin();
void loop();
void resetWifi();  // forget credentials and restart

}  // namespace net
