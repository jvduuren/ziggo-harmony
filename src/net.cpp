#include "net.h"

#include <ESPmDNS.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "ble_remote.h"
#include "bridge.h"
#include "config.h"
#include "keys.h"
#include "log.h"
#include "web_page.h"

namespace {

WiFiManager wifiManager;
WebServer web(80);
bool webStarted = false;

String jsonString(const String& s) {
  String out = "\"";
  for (char c : s) {
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (c == '\n') {
      out += "\\n";
    } else if ((uint8_t)c >= 0x20) {
      out += c;
    }
  }
  return out + "\"";
}

int keyArg() { return keymap::find(web.arg("key").c_str()); }

void sendOk() { web.send(200, "text/plain", "OK"); }

void sendUnknownKey() { web.send(404, "text/plain", "unknown key"); }

void handleStatus() {
  int learning = bridge::learningKey();
  int lastKey = bridge::lastCodeKey();
  String json = "{";
  json += "\"version\":" + jsonString(FW_VERSION);
  json += ",\"uptime\":" + String(millis() / 1000);
  json += ",\"ble\":{\"state\":" + jsonString(ble::stateText()) + ",\"peer\":" +
          jsonString(ble::peerAddress()) + ",\"address\":" + jsonString(ble::ownAddress()) +
          ",\"bonds\":" + String(ble::bondCount()) + "}";
  json += ",\"wifi\":{\"ssid\":" + jsonString(WiFi.SSID()) + ",\"ip\":" +
          jsonString(WiFi.localIP().toString()) + ",\"rssi\":" + String(WiFi.RSSI()) + "}";
  json += ",\"ir\":{\"last\":" + jsonString(ir::describe(bridge::lastCode())) + ",\"key\":" +
          (lastKey >= 0 ? jsonString(kKeys[lastKey].label) : String("null")) + "}";
  json += ",\"learning\":" + (learning >= 0 ? jsonString(kKeys[learning].label) : String("null"));
  json += ",\"box\":" + jsonString(ble::boxMessages());
  json += "}";
  web.send(200, "application/json", json);
}

void handleKeys() {
  String json = "[";
  for (size_t i = 0; i < kKeyCount; i++) {
    if (i) json += ",";
    json += "{\"id\":" + jsonString(kKeys[i].id) + ",\"label\":" + jsonString(kKeys[i].label) +
            ",\"ir\":" + jsonString(ir::describe(keymap::code(i))) + "}";
  }
  web.send(200, "application/json", json + "]");
}

void handleSend() {
  if (web.hasArg("report") && web.hasArg("usage")) {
    bridge::tapRaw(web.arg("report").toInt(), strtoul(web.arg("usage").c_str(), nullptr, 0));
    return sendOk();
  }
  int key = keyArg();
  if (key < 0) return sendUnknownKey();
  bridge::tap(key);
  sendOk();
}

void handleLearn() {
  if (!bridge::startLearning(keyArg())) return sendUnknownKey();
  sendOk();
}

void handleClear() {
  int key = keyArg();
  if (key < 0) return sendUnknownKey();
  keymap::clear(key);
  sendOk();
}

void handleUpdateDone() {
  bool ok = !Update.hasError();
  web.send(200, "text/plain", ok ? "Done, restarting..." : "Update failed");
  if (ok) {
    delay(500);
    ESP.restart();
  }
}

void handleUpdateUpload() {
  HTTPUpload& upload = web.upload();
  if (upload.status == UPLOAD_FILE_START) {
    logf("[ota] receiving %s\n", upload.filename.c_str());
    Update.begin(UPDATE_SIZE_UNKNOWN);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Update.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    Update.end(true);
    logf("[ota] %u bytes, %s\n", upload.totalSize, Update.hasError() ? "error" : "ok");
  }
}

void startWeb() {
  web.on("/", HTTP_GET, [] { web.send_P(200, "text/html", kIndexHtml); });
  web.on("/api/status", HTTP_GET, handleStatus);
  web.on("/api/keys", HTTP_GET, handleKeys);
  web.on("/api/log", HTTP_GET, [] { web.send(200, "text/plain", logText()); });
  web.on("/api/send", HTTP_POST, handleSend);
  web.on("/api/learn", HTTP_POST, handleLearn);
  web.on("/api/learn/cancel", HTTP_POST, [] {
    bridge::cancelLearning();
    sendOk();
  });
  web.on("/api/clear", HTTP_POST, handleClear);
  web.on("/api/clear-all", HTTP_POST, [] {
    keymap::clearAll();
    sendOk();
  });
  web.on("/api/map", HTTP_GET, [] { web.send(200, "text/plain", keymap::exportText()); });
  web.on("/api/map", HTTP_POST, [] { web.send(200, "text/plain", keymap::importText(web.arg("plain"))); });
  web.on("/api/unpair", HTTP_POST, [] {
    ble::unpair();
    sendOk();
  });
  web.on("/api/wifi-reset", HTTP_POST, [] {
    sendOk();
    delay(300);
    net::resetWifi();
  });
  web.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
  web.onNotFound([] { web.send(404, "text/plain", "not found"); });
  web.begin();

  MDNS.begin(kHostname);
  MDNS.addService("http", "tcp", 80);
  webStarted = true;
  logf("[web] http://%s/ or http://%s.local/\n", WiFi.localIP().toString().c_str(), kHostname);
}

}  // namespace

namespace net {

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(kHostname);
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setHostname(kHostname);
  // With saved credentials the portal only opens when the network is unreachable; close it
  // again after a few minutes and keep retrying the saved network.
  if (wifiManager.getWiFiIsSaved()) wifiManager.setConfigPortalTimeout(180);
  if (wifiManager.autoConnect(kSetupApName)) {
    logf("[wifi] connected to %s\n", WiFi.SSID().c_str());
  } else {
    logf("[wifi] setup portal: connect to Wi-Fi network '%s'\n", kSetupApName);
  }
}

void loop() {
  wifiManager.process();
  if (!webStarted && WiFi.status() == WL_CONNECTED && !wifiManager.getConfigPortalActive()) {
    startWeb();
  }
  if (webStarted) web.handleClient();
}

void resetWifi() {
  wifiManager.resetSettings();
  ESP.restart();
}

}  // namespace net
