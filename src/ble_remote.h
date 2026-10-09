#pragma once

#include <Arduino.h>

// BLE HID peripheral that presents itself exactly like the original
// Ziggo Next Mini remote (UEI "RemoteUnit").
namespace ble {

void begin();
bool connected();
bool ready();  // connected, encrypted and the box subscribed to key reports
void press(uint8_t report, uint16_t usage);
void release(uint8_t report);
void unpair();  // forget all bonds; the box can pair again
int bondCount();
String ownAddress();
String peerAddress();
const char* stateText();
String boxMessages();  // recent writes from the box (vendor report 8, HID control point)

}  // namespace ble
