#include "signalManager.h"
#include <Arduino.h>

Stream* SignalManager::serialStream = nullptr;
DynamicJsonDocument SignalManager::doc(256);

void SignalManager::setStream(Stream& stream) {
  serialStream = &stream;
}

void SignalManager::logSignal(const String& signalName, float signalValue) {
  if (serialStream == nullptr) {
    return; // Stream not set, exit early
  }

  doc.clear();
  doc[signalName] = serialized(String(signalValue,3));
  sendJson();
}

void SignalManager::logSignal(const String& signalName, char signalValue) {
  if (serialStream == nullptr) {
    return; // Stream not set, exit early
  }

  doc.clear();
  doc[signalName] = String(signalValue);
  sendJson();
}

void SignalManager::logSignal(const String& signalName, byte signalValue) {
  if (serialStream == nullptr) {
    return; // Stream not set, exit early
  }

  doc.clear();
  doc[signalName] = signalValue;
  sendJson();
}

void SignalManager::logSignal(const String& signalName, bool signalValue) {
  if (serialStream == nullptr) {
    return; // Stream not set, exit early
  }

  doc.clear();
  doc[signalName] = signalValue;
  sendJson();
}

void SignalManager::logSignal(const String& signalName, const char* signalValue) {
  if (serialStream == nullptr) {
    return; // Stream not set, exit early
  }

  doc.clear();
  doc[signalName] = signalValue;
  sendJson();
}

void SignalManager::sendJson() {
  if (serialStream == nullptr) {
    return; // Stream not set, exit early
  }

  String jsonString;
  serializeJson(doc, jsonString);
  serialStream->println(jsonString);
}
