#include <Arduino.h>
#include <Wire.h>
#include "at24c_async.h"

// Change these if your AT24C uses 1-byte memory addressing or a different I2C address.
static const uint8_t kAt24cI2cAddr = 0x50;
static const uint8_t kAt24cAddrBytes = 2;
static const uint16_t kTestAddress = 0x0000;
static const uint8_t kTestValue = 0xA5;
static const uint16_t kWriteSizeBytes = 1024;
static const uint8_t kPageSizeBytes = 16;
static const uint8_t kMaxChunkBytes = 8;

static At24cAsync g_eeprom(
    kAt24cI2cAddr, kAt24cAddrBytes, kPageSizeBytes, kMaxChunkBytes);
static uint8_t g_write_buffer[kWriteSizeBytes];
static uint8_t g_read_buffer[kWriteSizeBytes];
static bool g_reported = false;
static uint32_t g_idle_spins = 0;
static uint32_t g_start_us = 0;
static bool g_read_started = false;

void setup() {
  Serial.begin(115200);
  delay(1500);
  while (!Serial && millis() < 3000) {
    // Wait for serial monitor.
  }

  g_eeprom.Begin();
  Wire.setClock(400000);
  Serial.println("AT24C test starting...");

  uint8_t original = 0;
  if (!g_eeprom.ReadByte(kTestAddress, &original)) {
    Serial.println("Read failed (device not responding).");
    return;
  }

  Serial.print("Original value: 0x");
  Serial.println(original, HEX);

  uint8_t single = kTestValue;
  if (!g_eeprom.WriteByte(kTestAddress, single)) {
    Serial.println("Write failed.");
    return;
  }
  if (!g_eeprom.WaitReady(10)) {
    Serial.println("Write cycle timeout.");
    return;
  }

  uint8_t readback = 0;
  if (!g_eeprom.ReadByte(kTestAddress, &readback)) {
    Serial.println("Readback failed.");
    return;
  }

  Serial.print("Readback value: 0x");
  Serial.println(readback, HEX);
  Serial.println(readback == kTestValue ? "AT24C OK" : "AT24C MISMATCH");

  for (uint16_t i = 0; i < kWriteSizeBytes; ++i) {
    g_write_buffer[i] = static_cast<uint8_t>(i);
  }

  Serial.println("Starting non-blocking 1KB write...");
  g_eeprom.StartWrite(kTestAddress, g_write_buffer, kWriteSizeBytes);
  g_start_us = micros();
}

void loop() {
  g_eeprom.Service();
  g_idle_spins++;

  if (!g_read_started && g_eeprom.IsDone()) {
    g_read_started = true;
    g_eeprom.StartRead(kTestAddress, g_read_buffer, kWriteSizeBytes);
    return;
  }

  if (!g_reported && g_read_started && g_eeprom.IsDone()) {
    g_reported = true;
    const uint32_t total_us = micros() - g_start_us;
    const At24cAsync::Stats &stats = g_eeprom.GetStats();
    Serial.print("Total time (us): ");
    Serial.println(total_us);
    Serial.print("Max service time (us): ");
    Serial.println(stats.max_service_us);
    Serial.print("Write calls: ");
    Serial.println(stats.write_calls);
    Serial.print("Read calls: ");
    Serial.println(stats.read_calls);
    Serial.print("Poll calls: ");
    Serial.println(stats.poll_calls);
    Serial.print("Idle spins: ");
    Serial.println(g_idle_spins);

    bool match = true;
    for (uint16_t i = 0; i < kWriteSizeBytes; ++i) {
      if (g_read_buffer[i] != g_write_buffer[i]) {
        match = false;
        break;
      }
    }
    Serial.println(match ? "Async read OK" : "Async read mismatch");
  }

  if (g_eeprom.HasError()) {
    Serial.println("AT24C write error.");
    delay(1000);
  }
}
