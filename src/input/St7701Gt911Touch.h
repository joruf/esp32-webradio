#ifndef ST7701_GT911_TOUCH_H
#define ST7701_GT911_TOUCH_H

/**
 * @file St7701Gt911Touch.h
 * @brief GT911 touch on Waveshare ESP32-S3-Touch-LCD-2.8B (I2C, swipe volume).
 *
 * Factory GT911 config (read-only). Reset via TCA9554 EXIO2; INT is GPIO16.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>

#include "config/features_config.h"
#include "display/St7701Display.h"
#include "globals/app_prototypes.h"
#include "globals/globals.h"

uint8_t adjustVolume(int8_t delta);

extern volatile bool st7701UiVolumeDirty;

extern Arduino_GFX *tft;

#if defined(ST7701_28B)

#ifndef ST7701_TCA9554_ADDR
#define ST7701_TCA9554_ADDR 0x20
#endif

class St7701Gt911Touch {
public:
  static constexpr int8_t kIntPin = 16;
  static constexpr uint8_t kAddrPrimary = 0x5D;
  static constexpr uint8_t kAddrAlt = 0x14;
  static constexpr uint16_t kRegProductId = 0x8140;
  static constexpr uint16_t kRegStatus = 0x814E;
  static constexpr uint16_t kRegPoint0 = 0x8150;
  /** Minimum vertical travel before a swipe adjusts volume (ignore GT911 jitter). */
  static constexpr int16_t kSwipeMinPx = 56;
  /** Poll frames with contact before a lift can trigger volume. */
  static constexpr uint8_t kSwipeMinFrames = 3;
  /** GT911 status==0 streak while touchActive_: abort gesture without volume. */
  static constexpr uint8_t kIdleStatusAbortFrames = 4;
  static constexpr uint32_t kVolumeSwipeCooldownMs = 450;

  /**
   * @brief GT911 address-select reset on GPIO16 before VS1053 SPI claims that pin.
   */
  bool earlyHardwareReset() {
    Wire.begin(15, 7);
    Wire.setClock(400000);
    Wire.setTimeOut(25);
    if (!gt911HardwareReset_()) {
      return false;
    }
    hwResetDone_ = true;
    return true;
  }

  bool begin() {
    if (!hwResetDone_) {
      if (!earlyHardwareReset()) {
        Serial.println("GT911: TCA9554 reset failed");
        return false;
      }
    } else {
      Wire.begin(15, 7);
      Wire.setClock(400000);
      Wire.setTimeOut(25);
    }

    uint8_t addr = 0;
    if (probe_(kAddrPrimary)) {
      addr = kAddrPrimary;
    } else if (probe_(kAddrAlt)) {
      addr = kAddrAlt;
    } else {
      Serial.println("GT911: not found on I2C (0x5D / 0x14)");
      return false;
    }
    addr_ = addr;

    char pid[5] = {0, 0, 0, 0, 0};
    if (readReg_(kRegProductId, reinterpret_cast<uint8_t *>(pid), 4)) {
      Serial.printf("GT911: product ID \"%s\"\n", pid);
      if (strncmp(pid, "911", 3) != 0) {
        Serial.println("GT911: unexpected product ID (continuing anyway)");
      }
    }

    pinMode(kIntPin, INPUT_PULLUP);
    writeStatusClear_();
    Serial.printf("GT911: ready at 0x%02X (landscape %ux%u, INT GPIO%d)\n", addr_,
                  (unsigned)St7701Display::kLogicalW, (unsigned)St7701Display::kLogicalH, (int)kIntPin);
    return true;
  }

  /**
   * @brief Poll touch, map to landscape coords, handle vertical volume swipes.
   */
  void poll() {
    if (addr_ == 0) {
      return;
    }

    uint8_t status = 0;
    if (!readReg_(kRegStatus, &status, 1)) {
      return;
    }

    const uint8_t points = status & 0x0Fu;
    const bool bufferReady = (status & 0x80u) != 0;

    // After 0x814E clear the GT911 often reads 0 until the next frame. That is not finger-up.
    if (status == 0) {
      if (touchActive_) {
        if (++zeroStatusStreak_ >= kIdleStatusAbortFrames) {
          touchActive_ = false;
          touchFrameCount_ = 0;
          zeroStatusStreak_ = 0;
        }
      }
      return;
    }
    zeroStatusStreak_ = 0;

    if (bufferReady && points == 0) {
      if (touchActive_) {
        finishSwipe_();
        touchActive_ = false;
        touchFrameCount_ = 0;
      }
      writeStatusClear_();
      return;
    }

    if (!bufferReady || points == 0) {
      return;
    }

    uint8_t raw[4];
    if (!readReg_(kRegPoint0, raw, 4)) {
      writeStatusClear_();
      return;
    }

    const int16_t rawX = (int16_t)(raw[0] | (raw[1] << 8));
    const int16_t rawY = (int16_t)(raw[2] | (raw[3] << 8));
    mapToLandscape_(rawX, rawY, lastX_, lastY_);

    if (!touchActive_) {
      touchActive_ = true;
      startX_ = lastX_;
      startY_ = lastY_;
      touchFrameCount_ = 1;
      enableBacklight(true);
      bltimer = 0;
    } else {
      touchFrameCount_++;
    }

    writeStatusClear_();
  }

private:
  uint8_t addr_ = 0;
  bool hwResetDone_ = false;
  bool touchActive_ = false;
  int16_t startX_ = 0;
  int16_t startY_ = 0;
  int16_t lastX_ = 0;
  int16_t lastY_ = 0;
  uint8_t touchFrameCount_ = 0;
  uint8_t zeroStatusStreak_ = 0;
  uint32_t lastVolumeSwipeMs_ = 0;
  uint8_t tcaOut_ = 0xFF;

  bool tcaReadOutput_(uint8_t &outVal) {
    Wire.beginTransmission(ST7701_TCA9554_ADDR);
    Wire.write(0x01);
    if (Wire.endTransmission(false) != 0) {
      return false;
    }
    if (Wire.requestFrom((uint8_t)ST7701_TCA9554_ADDR, (uint8_t)1) != 1) {
      return false;
    }
    outVal = (uint8_t)Wire.read();
    tcaOut_ = outVal;
    return true;
  }

  bool tcaWriteOutput_(uint8_t outVal) {
    outVal &= (uint8_t)~(1u << 7); // EXIO8 buzzer off
    tcaOut_ = outVal;
    Wire.beginTransmission(ST7701_TCA9554_ADDR);
    Wire.write(0x01);
    Wire.write(tcaOut_);
    return Wire.endTransmission() == 0;
  }

  void tcaSetTpReset_(bool released) {
    const uint8_t bit = static_cast<uint8_t>(St7701ExioBit::ST7701_EXIO_TP_RST);
    if (released) {
      tcaOut_ |= (uint8_t)(1u << bit);
    } else {
      tcaOut_ &= (uint8_t)~(1u << bit);
    }
    tcaWriteOutput_(tcaOut_);
  }

  /**
   * GT911 address select: INT low during reset release -> I2C 0x5D (Waveshare default).
   */
  bool gt911HardwareReset_() {
    if (!tcaReadOutput_(tcaOut_)) {
      // Expander outputs default high; still try reset with safe mask.
      tcaOut_ = 0xFF;
      tcaOut_ &= (uint8_t)~(1u << 7);
    }

    pinMode(kIntPin, OUTPUT);
    digitalWrite(kIntPin, LOW);

    tcaSetTpReset_(false);
    delay(10);
    tcaSetTpReset_(true);
    delay(100);

    pinMode(kIntPin, INPUT);
    delay(10);
    return true;
  }

  bool probe_(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
  }

  bool readReg_(uint16_t reg, uint8_t *buf, size_t len) {
    Wire.beginTransmission(addr_);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)(reg & 0xFF));
    if (Wire.endTransmission(false) != 0) {
      return false;
    }
    const size_t got = Wire.requestFrom(addr_, len);
    if (got != len) {
      return false;
    }
    for (size_t i = 0; i < len; i++) {
      if (!Wire.available()) {
        return false;
      }
      buf[i] = (uint8_t)Wire.read();
    }
    return true;
  }

  void writeStatusClear_() {
    uint8_t zero = 0;
    Wire.beginTransmission(addr_);
    Wire.write((uint8_t)(kRegStatus >> 8));
    Wire.write((uint8_t)(kRegStatus & 0xFF));
    Wire.write(zero);
    Wire.endTransmission();
  }

  /**
   * GT911 reports portrait 480x640; GFX rotation 1 landscape is 640x480.
   * Matches Waveshare ST7701 MADCTL 0x60 + Arduino_GFX rotation 1.
   */
  void mapToLandscape_(int16_t rawX, int16_t rawY, int16_t &outX, int16_t &outY) {
    outX = rawY;
    outY = (int16_t)(St7701Display::kWidth - 1 - rawX);
    // Match GFX setRotation(3) when display_rotate_180 is enabled.
    if (radioFeatures.display_rotate_180) {
      outX = (int16_t)(St7701Display::kLogicalW - 1 - outX);
      outY = (int16_t)(St7701Display::kLogicalH - 1 - outY);
    }
    if (outX < 0) {
      outX = 0;
    }
    if (outY < 0) {
      outY = 0;
    }
    if (outX >= (int16_t)St7701Display::kLogicalW) {
      outX = (int16_t)St7701Display::kLogicalW - 1;
    }
    if (outY >= (int16_t)St7701Display::kLogicalH) {
      outY = (int16_t)St7701Display::kLogicalH - 1;
    }
  }

  void finishSwipe_() {
    if (touchFrameCount_ < kSwipeMinFrames) {
      return;
    }

    const int16_t dy = (int16_t)(lastY_ - startY_);
    const int16_t dx = (int16_t)(lastX_ - startX_);
    const int16_t absDy = (dy < 0) ? (int16_t)(-dy) : dy;
    const int16_t absDx = (dx < 0) ? (int16_t)(-dx) : dx;

    // Volume swipes are vertical only; ignore horizontal noise (zero dy with large dx).
    if (absDy < kSwipeMinPx || absDy <= absDx) {
      return;
    }

    const int16_t volDelta = (dy < 0) ? 5 : -5;
    const uint32_t now = millis();
    if (lastVolumeSwipeMs_ != 0 && (now - lastVolumeSwipeMs_) < kVolumeSwipeCooldownMs) {
      return;
    }
    lastVolumeSwipeMs_ = now;

    (void)adjustVolume((int8_t)volDelta);
    st7701UiVolumeDirty = true;
  }
};

#endif // ST7701_28B

#endif // ST7701_GT911_TOUCH_H
