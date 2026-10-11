// ziggo-harmony: IR (e.g. Logitech Harmony) to BLE bridge for the Ziggo Mediabox Next Mini.
// The ESP32 receives IR codes and presents itself to the box as the original BLE remote.

#include <Arduino.h>
#include <WiFi.h>

#include "ble_remote.h"
#include "bridge.h"
#include "config.h"
#include "ir_input.h"
#include "keys.h"
#include "led.h"
#include "log.h"
#include "net.h"

namespace {

void printHelp() {
  Serial.println(
      "commands: keys | send <key> | raw <report> <usage> | learn <key> | clear <key> |\n"
      "          map | status | unpair | scan [seconds] | wifi-reset | restart");
}

void printStatus() {
  Serial.printf("ziggo-harmony %s, box: %s %s, bonds: %d, wifi: %s\n", FW_VERSION, ble::stateText(),
                ble::peerAddress().c_str(), ble::bondCount(),
                WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "-");
}

void runCommand(String line) {
  line.trim();
  int space = line.indexOf(' ');
  String cmd = space < 0 ? line : line.substring(0, space);
  String arg = space < 0 ? String() : line.substring(space + 1);
  arg.trim();

  if (cmd == "keys") {
    for (size_t i = 0; i < kKeyCount; i++) {
      String codes = keymap::describe(i);
      codes.replace("\n", ", ");
      Serial.printf("  %-11s %-13s %s\n", kKeys[i].id, kKeys[i].label, codes.c_str());
    }
  } else if (cmd == "send" || cmd == "learn" || cmd == "clear") {
    int key = keymap::find(arg.c_str());
    if (key < 0) {
      Serial.println("unknown key, see 'keys'");
    } else if (cmd == "send") {
      bridge::tap(key);
    } else if (cmd == "learn") {
      bridge::startLearning(key);
    } else {
      keymap::clear(key);
    }
  } else if (cmd == "raw") {
    unsigned report = 0, usage = 0;
    if (sscanf(arg.c_str(), "%u %i", &report, (int*)&usage) == 2) {
      bridge::tapRaw(report, usage);
    } else {
      Serial.println("usage: raw <report> <usage>, e.g. raw 3 0x40");
    }
  } else if (cmd == "map") {
    Serial.print(keymap::exportText());
  } else if (cmd == "status") {
    printStatus();
  } else if (cmd == "unpair") {
    ble::unpair();
  } else if (cmd == "scan") {
    ble::scan(arg.length() ? arg.toInt() : 60);
  } else if (cmd == "wifi-reset") {
    net::resetWifi();
  } else if (cmd == "restart") {
    ESP.restart();
  } else if (cmd.length()) {
    printHelp();
  }
}

void pollConsole() {
  static String line;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (line.length()) runCommand(line);
      line = "";
    } else if (line.length() < 80) {
      line += c;
    }
  }
}

// Hold BOOT for 5 s to unpair the box, 10 s to also forget Wi-Fi.
void pollButton() {
  static uint32_t pressedAt = 0;
  bool down = digitalRead(BOOT_BUTTON_PIN) == LOW;
  uint32_t now = millis();
  if (down && !pressedAt) pressedAt = now;
  if (down && pressedAt && now - pressedAt >= kUnpairHoldMs) {
    led::flash(30, 0, 0, 100);  // feedback: long-press threshold reached
  }
  if (!down && pressedAt) {
    uint32_t held = now - pressedAt;
    pressedAt = 0;
    if (held >= kWifiResetHoldMs) {
      ble::unpair();
      net::resetWifi();
    } else if (held >= kUnpairHoldMs) {
      ble::unpair();
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);  // never let logging stall the bridge when no terminal is reading
  delay(500);
  logf("ziggo-harmony %s\n", FW_VERSION);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  led::begin();
  led::selfTest();
  keymap::load();
  ble::begin();
  ir::begin();
  net::begin();
  printHelp();
}

void loop() {
  IrEvent event;
  while (ir::poll(event)) bridge::handleIr(event);
  bridge::tick();

  net::loop();
  pollConsole();
  pollButton();

  led::setMode(bridge::learningKey() >= 0 ? led::Mode::Learning
               : ble::ready()             ? led::Mode::Connected
                                          : led::Mode::Advertising);
  led::update();
  delay(1);
}
