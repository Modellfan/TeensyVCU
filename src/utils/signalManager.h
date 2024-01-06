#ifndef SIGNALMANAGER_H
#define SIGNALMANAGER_H

#include <Arduino.h> // Include this header for the String type
#include <ArduinoJson.h>

class SignalManager {
public:
  static void setStream(Stream& stream);
  static void logSignal(const String& signalName, float signalValue);
  static void logSignal(const String& signalName, char signalValue);
  static void logSignal(const String& signalName, byte signalValue);
  static void logSignal(const String& signalName, bool signalValue);
  static void logSignal(const String& signalName, const char* signalValue);

private:
  static Stream* serialStream;
  static DynamicJsonDocument doc;
  static void sendJson();
};

#endif // SIGNALMANAGER_H
