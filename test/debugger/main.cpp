#include <Arduino.h>
#include <math.h>

// Globals for debugger watch window.
float g_sin_a = 0.0f;
float g_sin_b = 0.0f;

static const float kTwoPi = 6.28318530718f;
static const float kFreqAHz = 0.5f;
static const float kFreqBHz = 0.25f;
static const float kPhaseB = 1.57079632679f;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    // Wait for serial monitor.
  }
}

void loop() {
  const float t = millis() / 1000.0f;
  g_sin_a = sinf(kTwoPi * kFreqAHz * t);
  g_sin_b = sinf(kTwoPi * kFreqBHz * t + kPhaseB);
  delay(10);
}