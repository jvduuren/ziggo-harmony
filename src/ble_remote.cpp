#include "ble_remote.h"

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#include <mutex>

#include "config.h"
#include "led.h"

namespace {

// HID report map, byte for byte identical to the original remote (docs/ble-profile.md).
const uint8_t kReportMap[] = {
    // Report 1: Keypad, 2 x 16-bit usages from the Keyboard/Keypad page
    0x05, 0x01, 0x09, 0x07, 0xA1, 0x01, 0x85, 0x01, 0x95, 0x02, 0x75, 0x10, 0x05, 0x07,
    0x19, 0x00, 0x2A, 0x00, 0x01, 0x15, 0x00, 0x26, 0x00, 0x01, 0x81, 0x00, 0xC0,
    // Report 3: Consumer Control, 2 x 16-bit usages
    0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x03, 0x95, 0x02, 0x75, 0x10, 0x19, 0x00,
    0x2A, 0xFF, 0x03, 0x15, 0x00, 0x26, 0xFF, 0x03, 0x81, 0x00, 0xC0,
    // Report 2: System Control, 2 x 16-bit usages
    0x05, 0x01, 0x09, 0x80, 0xA1, 0x01, 0x85, 0x02, 0x95, 0x02, 0x75, 0x10, 0x19, 0x00,
    0x2A, 0x00, 0x01, 0x15, 0x00, 0x26, 0x00, 0x01, 0x81, 0x00, 0xC0,
    // Report 7: vendor input, 197 bytes (voice / UEI channel)
    0x06, 0x01, 0xFF, 0x09, 0x01, 0xA1, 0x01, 0xA1, 0x02, 0x85, 0x07, 0x09, 0x14, 0x15,
    0x80, 0x25, 0x7F, 0x75, 0x08, 0x95, 0xC5, 0x81, 0x22, 0xC0, 0xC0,
    // Report 8: vendor output, 197 bytes (box -> remote)
    0x06, 0x01, 0xFF, 0x09, 0x01, 0xA1, 0x01, 0xA1, 0x02, 0x85, 0x08, 0x09, 0x14, 0x15,
    0x80, 0x25, 0x7F, 0x75, 0x08, 0x95, 0xC5, 0x91, 0x22, 0xC0, 0xC0,
    // Report 0x1F: vendor input, 160 bytes
    0x06, 0x01, 0xFF, 0x09, 0x04, 0xA1, 0x01, 0xA1, 0x02, 0x85, 0x1F, 0x09, 0x14, 0x15,
    0x80, 0x25, 0x7F, 0x75, 0x08, 0x95, 0xA0, 0x81, 0x22, 0xC0, 0xC0,
};

// Manufacturer data in the advertisement: company 0x0093 (Universal Electronics), data 00 80.
const uint8_t kManufacturerData[] = {0x93, 0x00, 0x00, 0x80};

NimBLEServer* server = nullptr;
NimBLEHIDDevice* hid = nullptr;
NimBLECharacteristic* keypadReport = nullptr;
NimBLECharacteristic* systemReport = nullptr;
NimBLECharacteristic* consumerReport = nullptr;

volatile bool isConnected = false;
volatile bool isEncrypted = false;
volatile uint8_t subscribedMask = 0;  // bit n = box subscribed to report n
volatile uint16_t connHandle = 0;
String peer;

constexpr size_t kMaxBoxMessages = 8;
String boxLog[kMaxBoxMessages];
size_t boxLogNext = 0;
std::mutex stateMutex;

String toHex(const uint8_t* data, size_t len) {
  String s;
  s.reserve(len * 3);
  char buf[4];
  for (size_t i = 0; i < len; i++) {
    snprintf(buf, sizeof(buf), i ? " %02X" : "%02X", data[i]);
    s += buf;
  }
  return s;
}

void logBoxMessage(const char* what, const NimBLEAttValue& value) {
  String line = String(what) + ": " + toHex(value.data(), value.size());
  Serial.printf("[ble] box -> remote %s\n", line.c_str());
  std::lock_guard<std::mutex> lock(stateMutex);
  boxLog[boxLogNext] = line;
  boxLogNext = (boxLogNext + 1) % kMaxBoxMessages;
}

NimBLECharacteristic* reportCharacteristic(uint8_t report) {
  switch (report) {
    case 1: return keypadReport;
    case 2: return systemReport;
    case 3: return consumerReport;
    default: return nullptr;
  }
}

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*, NimBLEConnInfo& info) override {
    isConnected = true;
    connHandle = info.getConnHandle();
    std::string address = info.getAddress().toString();
    {
      std::lock_guard<std::mutex> lock(stateMutex);
      peer = address.c_str();
    }
    Serial.printf("[ble] connected: %s\n", address.c_str());
  }

  void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason) override {
    isConnected = false;
    isEncrypted = false;
    subscribedMask = 0;
    Serial.printf("[ble] disconnected, reason 0x%02X\n", reason);
  }

  void onAuthenticationComplete(NimBLEConnInfo& info) override {
    isEncrypted = info.isEncrypted();
    Serial.printf("[ble] security: encrypted=%d bonded=%d\n", info.isEncrypted(), info.isBonded());
  }
};

class ReportCallbacks : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic* chr, NimBLEConnInfo&, uint16_t subValue) override {
    uint8_t report = chr == keypadReport ? 1 : chr == systemReport ? 2 : chr == consumerReport ? 3 : 0;
    if (!report) return;
    if (subValue) {
      subscribedMask |= 1 << report;
    } else {
      subscribedMask &= ~(1 << report);
    }
    Serial.printf("[ble] report %u %s\n", report, subValue ? "subscribed" : "unsubscribed");
  }
};

class LogWriteCallbacks : public NimBLECharacteristicCallbacks {
 public:
  explicit LogWriteCallbacks(const char* what) : what_(what) {}
  void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo&) override {
    logBoxMessage(what_, chr->getValue());
  }

 private:
  const char* what_;
};

class AlertCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo&) override {
    NimBLEAttValue v = chr->getValue();
    uint8_t level = v.size() ? v.data()[0] : 0;
    Serial.printf("[ble] find-my-remote alert, level %u\n", level);
    led::findMe(level ? 5000 : 0);
  }
};

ServerCallbacks serverCallbacks;
ReportCallbacks reportCallbacks;
LogWriteCallbacks vendorOutCallbacks("vendor report 8");
LogWriteCallbacks hidControlCallbacks("HID control point");
LogWriteCallbacks linkLossCallbacks("link loss alert level");
AlertCallbacks alertCallbacks;

void addReadOnly(NimBLEService* svc, uint16_t uuid, const uint8_t* data, size_t len) {
  svc->createCharacteristic(uuid, NIMBLE_PROPERTY::READ)->setValue(data, len);
}

void addReadOnly(NimBLEService* svc, uint16_t uuid, const char* text) {
  addReadOnly(svc, uuid, reinterpret_cast<const uint8_t*>(text), strlen(text));
}

}  // namespace

namespace ble {

void begin() {
  NimBLEDevice::init(kBleName);
  NimBLEDevice::setSecurityAuth(true, false, true);  // bonding, no MITM, secure connections
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  server = NimBLEDevice::createServer();
  server->setCallbacks(&serverCallbacks, false);
  server->advertiseOnDisconnect(true);

  hid = new NimBLEHIDDevice(server);
  hid->setManufacturer("Universal Electronics, Inc.");
  hid->setPnp(0x02, 0x06E7, 0x8140, 0x0110);
  hid->setHidInfo(0x00, 0x03);  // remote wake + normally connectable
  hid->setReportMap(const_cast<uint8_t*>(kReportMap), sizeof(kReportMap));
  hid->setBatteryLevel(100);

  keypadReport = hid->getInputReport(1);
  systemReport = hid->getInputReport(2);
  consumerReport = hid->getInputReport(3);
  keypadReport->setCallbacks(&reportCallbacks);
  systemReport->setCallbacks(&reportCallbacks);
  consumerReport->setCallbacks(&reportCallbacks);
  hid->getInputReport(7);
  hid->getInputReport(0x1F);
  hid->getOutputReport(8)->setCallbacks(&vendorOutCallbacks);
  hid->getHidControl()->setCallbacks(&hidControlCallbacks);

  // Remaining Device Information, as read from the original remote. The System ID is
  // derived from our own address, like the original does.
  NimBLEService* dis = hid->getDeviceInfoService();
  const uint8_t* a = NimBLEDevice::getAddress().getVal();
  const uint8_t systemId[] = {a[0], a[1], a[2], 0xFF, 0xFE, a[3], a[4], a[5]};
  const uint8_t regCert[] = {0x33, 0x00, 0x00, 0x00, 0x00, 0x00};
  addReadOnly(dis, 0x2A23, systemId, sizeof(systemId));
  addReadOnly(dis, 0x2A24, "RemoteUnit");
  addReadOnly(dis, 0x2A25, "0000-0001");
  addReadOnly(dis, 0x2A26, "BL 196");
  addReadOnly(dis, 0x2A27, "01");
  addReadOnly(dis, 0x2A28, "7060.29.00_0.0");
  addReadOnly(dis, 0x2A2A, regCert, sizeof(regCert));

  const uint8_t zero = 0;
  NimBLEService* linkLoss = server->createService((uint16_t)0x1803);
  NimBLECharacteristic* linkLossLevel = linkLoss->createCharacteristic(
      (uint16_t)0x2A06, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  linkLossLevel->setValue(&zero, 1);
  linkLossLevel->setCallbacks(&linkLossCallbacks);
  NimBLEService* immediateAlert = server->createService((uint16_t)0x1802);
  immediateAlert->createCharacteristic((uint16_t)0x2A06, NIMBLE_PROPERTY::WRITE_NR)
      ->setCallbacks(&alertCallbacks);
  NimBLEService* txPower = server->createService((uint16_t)0x1804);
  addReadOnly(txPower, 0x2A07, &zero, 1);

  server->start();

  NimBLEAdvertisementData adv;
  adv.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
  adv.setName(kBleName);
  adv.setCompleteServices16({NimBLEUUID((uint16_t)0x1812), NimBLEUUID((uint16_t)0x180F)});
  adv.setManufacturerData(kManufacturerData, sizeof(kManufacturerData));
  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->setAdvertisementData(adv);
  advertising->start();

  Serial.printf("[ble] advertising as %s (%s), %d bond(s)\n", kBleName, ownAddress().c_str(),
                bondCount());
}

bool connected() { return isConnected; }

bool ready() { return isConnected && isEncrypted && subscribedMask; }

void press(uint8_t report, uint16_t usage) {
  NimBLECharacteristic* chr = reportCharacteristic(report);
  if (!chr || !isConnected) return;
  const uint8_t data[4] = {uint8_t(usage & 0xFF), uint8_t(usage >> 8), 0, 0};
  chr->setValue(data, sizeof(data));
  chr->notify();
}

void release(uint8_t report) {
  NimBLECharacteristic* chr = reportCharacteristic(report);
  if (!chr || !isConnected) return;
  const uint8_t data[4] = {0, 0, 0, 0};
  chr->setValue(data, sizeof(data));
  chr->notify();
}

void unpair() {
  Serial.println("[ble] deleting all bonds");
  if (isConnected) server->disconnect(connHandle);
  NimBLEDevice::deleteAllBonds();
}

int bondCount() { return NimBLEDevice::getNumBonds(); }

String ownAddress() { return NimBLEDevice::getAddress().toString().c_str(); }

String peerAddress() {
  std::lock_guard<std::mutex> lock(stateMutex);
  return isConnected ? peer : String();
}

const char* stateText() {
  if (ready()) return "connected";
  if (isConnected) return "connecting";
  return "advertising";
}

String boxMessages() {
  String out;
  std::lock_guard<std::mutex> lock(stateMutex);
  for (size_t i = 0; i < kMaxBoxMessages; i++) {
    const String& line = boxLog[(boxLogNext + i) % kMaxBoxMessages];
    if (line.length()) out += line + "\n";
  }
  return out;
}

}  // namespace ble
