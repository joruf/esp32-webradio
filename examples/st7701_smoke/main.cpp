/**
 * Minimal ST7701 RGB smoke test for Waveshare ESP32-S3-Touch-LCD-2.8B.
 * Cycles full-screen colors and prints "alive" on USB CDC serial.
 */

#include <Arduino.h>
#include "display/St7701Display.h"

static St7701Display gDisplay;

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println("ST7701 smoke: boot");
  if (!gDisplay.begin()) {
    Serial.println("ST7701 smoke: display begin FAILED");
  } else {
    Serial.println("ST7701 smoke: display begin ok");
  }
}

void loop() {
  static uint32_t lastMs = 0;
  static uint8_t step = 0;
  const uint32_t now = millis();

  if (now - lastMs < 2000) {
    return;
  }
  lastMs = now;

  static const uint16_t kColors[] = {0xF800, 0x07E0, 0x001F, 0xFFFF};
  static const char *kNames[] = {"RED", "GREEN", "BLUE", "WHITE"};
  const uint8_t idx = step % 4;

  Arduino_GFX *gfx = gDisplay.gfx();
  if (gfx) {
    gfx->fillScreen(kColors[idx]);
  }

  Serial.printf("alive millis=%lu color=%s\n", (unsigned long)now, kNames[idx]);
  step++;
}
