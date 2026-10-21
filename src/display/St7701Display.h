#ifndef ST7701_DISPLAY_H
#define ST7701_DISPLAY_H

/**
 * @file St7701Display.h
 * @brief Waveshare ESP32-S3-Touch-LCD-2.8B panel bring-up (ST7701 + RGB565).
 *
 * Init path: TCA9554 EXIO for LCD RST/CS, 3-wire SPI on GPIO1/2, then
 * Arduino_GFX RGB panel for framebuffer drawing.
 */

#include <Arduino.h>
#include <Wire.h>
#include <esp_task_wdt.h>
#include <Arduino_GFX_Library.h>
#include "st7701_rgb_flush.h"

#if defined(ST7701_28B)

#ifndef ST7701_TCA9554_ADDR
#define ST7701_TCA9554_ADDR 0x20
#endif

/** TCA9554 bit map on Waveshare 2.8B (EXIO1..). */
enum St7701ExioBit : uint8_t {
  ST7701_EXIO_LCD_RST = 0, // EXIO1
  ST7701_EXIO_TP_RST = 1,  // EXIO2
  ST7701_EXIO_LCD_CS = 2,  // EXIO3
  ST7701_EXIO_SD_CS = 3,   // EXIO4
  ST7701_EXIO_BUZZER = 7,  // EXIO8, onboard piezo (active high)
};

class St7701Display {
public:
  /** Physical RGB scan order (portrait). */
  static constexpr uint16_t kWidth = 480;
  static constexpr uint16_t kHeight = 640;
  /** Logical size after rotation 1 (landscape). */
  static constexpr uint16_t kLogicalW = 640;
  static constexpr uint16_t kLogicalH = 480;

  St7701Display() = default;

  /**
   * @brief Initialize expander, ST7701 SPI config, backlight and RGB panel.
   * @return true when gfx() is ready for drawing
   */
  bool begin() {
    Serial.println("ST7701: begin (TCA9554 + SPI + RGB)");
    Wire.begin(15, 7);
    Wire.setClock(400000);
    if (!tcaBegin_()) {
      Serial.println("ST7701: TCA9554 not found at 0x20");
      return false;
    }
    Serial.println("ST7701: TCA9554 ok");

    // Hold touch reset released so GT911 stays quiet during LCD bring-up.
    tcaWriteBit_(ST7701_EXIO_TP_RST, true);
    tcaWriteBit_(ST7701_EXIO_LCD_CS, true);
    tcaWriteBit_(ST7701_EXIO_LCD_RST, false);
    delay(20);
    tcaWriteBit_(ST7701_EXIO_LCD_RST, true);
    delay(120);
    esp_task_wdt_reset();

    // Keep LCD CS low through SPI init AND RGB panel create/init (Waveshare order).
    st7701SpiInit_();
    Serial.println("ST7701: SPI init done (CS still low)");

    // Bounce lines from PSRAM FB: DMA copies line chunks (not UI text scroll).
    // bounce=0 fixed horizontal tear but exposed vertical banding when flush raced the scan.
    // Arduino_GFX keeps double_fb=false / bb_invalidate_cache=false; only bounce size is tunable here.
    static constexpr size_t kBounceLines = 10;
    rgbpanel_ = new Arduino_ESP32RGBPanel(
        kDePin /* DE */, kVsyncPin /* VSYNC */, kHsyncPin /* HSYNC */, kPclkPin /* PCLK */,
        kR0 /* R0 */, kR1 /* R1 */, kR2 /* R2 */, kR3 /* R3 */, kR4 /* R4 */,
        kG0 /* G0 */, kG1 /* G1 */, kG2 /* G2 */, kG3 /* G3 */, kG4 /* G4 */, kG5 /* G5 */,
        kB0 /* B0 */, kB1 /* B1 */, kB2 /* B2 */, kB3 /* B3 */, kB4 /* B4 */,
        1 /* hsync_polarity */, 50 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 10 /* hsync_back_porch */,
        1 /* vsync_polarity */, 8 /* vsync_front_porch */, 2 /* vsync_pulse_width */, 18 /* vsync_back_porch */,
        // Stable 2.8B timing: 16 MHz PCLK + bounce (30 MHz / VSYNC GPIO broke scanout).
        1 /* pclk_active_neg */, 16000000 /* prefer_speed */, false /* useBigEndian */,
        0 /* de_idle_high */, 0 /* pclk_idle_high */,
        (size_t)(kWidth * kBounceLines) /* bounce_buffer_size_px */);

    if (!rgbpanel_) {
      spiCs_(true);
      Serial.println("ST7701: RGB panel alloc failed");
      return false;
    }

    // Landscape (rotation 1). auto_flush off: one cache writeback per frame in dsp_update().
    gfx_ = new Arduino_RGB_Display(kWidth, kHeight, rgbpanel_, 1 /* rotation */, false /* auto_flush */,
                                   nullptr, GFX_NOT_DEFINED, nullptr, 0);
    if (!gfx_) {
      spiCs_(true);
      Serial.println("ST7701: GFX alloc failed");
      return false;
    }
    esp_task_wdt_reset();
    if (!gfx_->begin()) {
      spiCs_(true);
      Serial.println("ST7701: RGB panel begin failed");
      return false;
    }
    esp_task_wdt_reset();

    // Release CS only after RGB peripheral is running.
    spiCs_(true);
    delay(10);

    initBacklightPwm_();
    setBacklightLevel(100);

    gfx_->fillScreen(0x0000);
    // Panel needs one DMA kick; dashboard frames flush only from spftask after this.
    st7701FlushRgbFramebuffer(gfx_);
    esp_task_wdt_reset();
    Serial.println("ST7701: dsp_begin ok (RGB landscape + backlight)");
    return true;
  }

  Arduino_GFX *gfx() {
    return gfx_;
  }

  /** Backlight 0..100 percent on GPIO6 (LEDC). */
  void setBacklightLevel(uint8_t percent) {
    if (percent > 100) {
      percent = 100;
    }
    backlightLevel_ = percent;
    if (!backlightPwmReady_) {
      digitalWrite(kBlPin, percent > 0 ? HIGH : LOW);
      return;
    }
    const uint32_t duty = (uint32_t)percent * 8191U / 100U;
    ledcWrite(kBlPin, duty);
  }

  uint8_t backlightLevel() const {
    return backlightLevel_;
  }

private:
  static constexpr int8_t kBlPin = 6;
  bool backlightPwmReady_ = false;
  uint8_t backlightLevel_ = 100;

  void initBacklightPwm_() {
    pinMode(kBlPin, OUTPUT);
    if (ledcAttach(kBlPin, 4000, 13)) {
      backlightPwmReady_ = true;
      return;
    }
    backlightPwmReady_ = false;
    digitalWrite(kBlPin, HIGH);
  }
  static constexpr int8_t kSpiSda = 1; // LCD_SDA / MOSI
  static constexpr int8_t kSpiScl = 2; // LCD_SCL / SCK
  static constexpr int8_t kDePin = 40;
  static constexpr int8_t kVsyncPin = 39;
  static constexpr int8_t kHsyncPin = 38;
  static constexpr int8_t kPclkPin = 41;
  // RGB565 mapping for Waveshare 2.8B (R0/B0 tied NC -> reuse neighbour)
  static constexpr int8_t kR0 = 46, kR1 = 3, kR2 = 8, kR3 = 18, kR4 = 17;
  static constexpr int8_t kG0 = 14, kG1 = 13, kG2 = 12, kG3 = 11, kG4 = 10, kG5 = 9;
  static constexpr int8_t kB0 = 5, kB1 = 45, kB2 = 48, kB3 = 47, kB4 = 21;

  Arduino_ESP32RGBPanel *rgbpanel_ = nullptr;
  Arduino_RGB_Display *gfx_ = nullptr;
  uint8_t tcaOut_ = 0xFF;

  bool tcaBegin_() {
    Wire.beginTransmission(ST7701_TCA9554_ADDR);
    if (Wire.endTransmission() != 0) {
      return false;
    }
    // All pins output
    Wire.beginTransmission(ST7701_TCA9554_ADDR);
    Wire.write(0x03);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) {
      return false;
    }
    tcaOut_ = 0xFF;
    tcaOut_ &= (uint8_t)~(1u << ST7701_EXIO_BUZZER);
    Wire.beginTransmission(ST7701_TCA9554_ADDR);
    Wire.write(0x01);
    Wire.write(tcaOut_);
    return Wire.endTransmission() == 0;
  }

  void tcaWriteBit_(uint8_t bit, bool level) {
    if (level) {
      tcaOut_ |= (uint8_t)(1u << bit);
    } else {
      tcaOut_ &= (uint8_t)~(1u << bit);
    }
    Wire.beginTransmission(ST7701_TCA9554_ADDR);
    Wire.write(0x01);
    Wire.write(tcaOut_);
    Wire.endTransmission();
  }

  void spiCs_(bool level) {
    tcaWriteBit_(ST7701_EXIO_LCD_CS, level);
  }

  void spiWrite9_(uint16_t bits) {
    for (int i = 8; i >= 0; i--) {
      digitalWrite(kSpiScl, LOW);
      digitalWrite(kSpiSda, (bits >> i) & 0x1);
      digitalWrite(kSpiScl, HIGH);
    }
  }

  // Caller holds CS low for the whole SPI sequence (and until RGB begin finishes).
  void spiCmd_(uint8_t cmd) {
    spiWrite9_((uint16_t)cmd); // D/CX = 0
  }

  void spiData_(uint8_t data) {
    spiWrite9_((uint16_t)0x100 | data); // D/CX = 1
  }

  void spiDataBuf_(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
      spiData_(data[i]);
    }
  }

  void delayWithWdt_(uint32_t ms) {
    const uint32_t end = millis() + ms;
    while ((int32_t)(millis() - end) < 0) {
      esp_task_wdt_reset();
      delay(40);
    }
  }

  /**
   * @brief ST7701 bring-up sequence from Waveshare ESP32-S3-Touch-LCD-2.8B demo.
   * Leaves LCD CS asserted (low); caller releases it after RGB panel init.
   */
  void st7701SpiInit_() {
    pinMode(kSpiSda, OUTPUT);
    pinMode(kSpiScl, OUTPUT);
    digitalWrite(kSpiScl, HIGH);
    digitalWrite(kSpiSda, HIGH);

    spiCs_(false);
    delay(10);

    spiCmd_(0xFF);
    spiDataBuf_((const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5);
    spiCmd_(0xEF);
    spiData_(0x08);
    spiCmd_(0xFF);
    spiDataBuf_((const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5);
    spiCmd_(0xC0);
    spiDataBuf_((const uint8_t[]){0x4F, 0x00}, 2);
    spiCmd_(0xC1);
    spiDataBuf_((const uint8_t[]){0x10, 0x02}, 2);
    spiCmd_(0xC2);
    spiDataBuf_((const uint8_t[]){0x07, 0x02}, 2);
    spiCmd_(0xCC);
    spiData_(0x10);
    spiCmd_(0xB0);
    spiDataBuf_((const uint8_t[]){0x00, 0x10, 0x17, 0x0D, 0x11, 0x06, 0x05, 0x08, 0x07, 0x1F, 0x04, 0x11, 0x0E,
                                  0x29, 0x30, 0x1F},
                16);
    spiCmd_(0xB1);
    spiDataBuf_((const uint8_t[]){0x00, 0x0D, 0x14, 0x0E, 0x11, 0x06, 0x04, 0x08, 0x08, 0x20, 0x05, 0x13, 0x13,
                                  0x26, 0x30, 0x1F},
                16);

    spiCmd_(0xFF);
    spiDataBuf_((const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5);
    spiCmd_(0xB0);
    spiData_(0x65);
    spiCmd_(0xB1);
    spiData_(0x71);
    spiCmd_(0xB2);
    spiData_(0x82);
    spiCmd_(0xB3);
    spiData_(0x80);
    spiCmd_(0xB5);
    spiData_(0x42);
    spiCmd_(0xB7);
    spiData_(0x85);
    spiCmd_(0xB8);
    spiData_(0x20);
    spiCmd_(0xC0);
    spiData_(0x09);
    spiCmd_(0xC1);
    spiData_(0x78);
    spiCmd_(0xC2);
    spiData_(0x78);
    spiCmd_(0xD0);
    spiData_(0x88);
    spiCmd_(0xEE);
    spiData_(0x42);

    spiCmd_(0xE0);
    spiDataBuf_((const uint8_t[]){0x00, 0x00, 0x02}, 3);
    spiCmd_(0xE1);
    spiDataBuf_((const uint8_t[]){0x04, 0xA0, 0x06, 0xA0, 0x05, 0xA0, 0x07, 0xA0, 0x00, 0x44, 0x44}, 11);
    spiCmd_(0xE2);
    spiDataBuf_((const uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 12);
    spiCmd_(0xE3);
    spiDataBuf_((const uint8_t[]){0x00, 0x00, 0x22, 0x22}, 4);
    spiCmd_(0xE4);
    spiDataBuf_((const uint8_t[]){0x44, 0x44}, 2);
    spiCmd_(0xE5);
    spiDataBuf_((const uint8_t[]){0x0C, 0x90, 0xA0, 0xA0, 0x0E, 0x92, 0xA0, 0xA0, 0x08, 0x8C, 0xA0, 0xA0, 0x0A,
                                  0x8E, 0xA0, 0xA0},
                16);
    spiCmd_(0xE6);
    spiDataBuf_((const uint8_t[]){0x00, 0x00, 0x22, 0x22}, 4);
    spiCmd_(0xE7);
    spiDataBuf_((const uint8_t[]){0x44, 0x44}, 2);
    spiCmd_(0xE8);
    spiDataBuf_((const uint8_t[]){0x0D, 0x91, 0xA0, 0xA0, 0x0F, 0x93, 0xA0, 0xA0, 0x09, 0x8D, 0xA0, 0xA0, 0x0B,
                                  0x8F, 0xA0, 0xA0},
                16);
    spiCmd_(0xEB);
    spiDataBuf_((const uint8_t[]){0x00, 0x00, 0xE4, 0xE4, 0x44, 0x00, 0x40}, 7);
    // MADCTL: landscape (MV|MX) to match GFX rotation 1 and GT911 mapping.
    spiCmd_(0x36);
    spiData_(0x60);
    spiCmd_(0xED);
    spiDataBuf_((const uint8_t[]){0xFF, 0xF5, 0x47, 0x6F, 0x0B, 0xA1, 0xAB, 0xFF, 0xFF, 0xBA, 0x1A, 0xB0, 0xF6,
                                  0x74, 0x5F, 0xFF},
                16);
    spiCmd_(0xEF);
    spiDataBuf_((const uint8_t[]){0x08, 0x08, 0x08, 0x40, 0x3F, 0x64}, 6);

    spiCmd_(0xFF);
    spiDataBuf_((const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5);
    spiCmd_(0xFF);
    spiDataBuf_((const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5);
    spiCmd_(0xE6);
    spiDataBuf_((const uint8_t[]){0x16, 0x7C}, 2);
    spiCmd_(0xE8);
    spiDataBuf_((const uint8_t[]){0x00, 0x0E}, 2);
    spiCmd_(0xFF);
    spiDataBuf_((const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5);
    spiCmd_(0x11);
    spiData_(0x00);
    delayWithWdt_(200);
    spiCmd_(0xFF);
    spiDataBuf_((const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5);
    spiCmd_(0xE8);
    spiDataBuf_((const uint8_t[]){0x00, 0x0C}, 2);
    delayWithWdt_(150);
    esp_task_wdt_reset();
    spiCmd_(0xE8);
    spiDataBuf_((const uint8_t[]){0x00, 0x00}, 2);
    spiCmd_(0xFF);
    spiDataBuf_((const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5);
    spiCmd_(0x29);
    spiData_(0x00);
    spiCmd_(0x35);
    spiData_(0x00);
    spiCmd_(0x11);
    spiData_(0x00);
    delayWithWdt_(200);
    spiCmd_(0x29);
    spiData_(0x00);
    spiCmd_(0x29);
    spiData_(0x00);
    delayWithWdt_(100);
    esp_task_wdt_reset();

    // Disable ST7701 vertical scroll (VSCRDEF / VSCRSADD): TFA=0, VSA=640, BFA=0, start=0.
    // Avoids panel-side line offset that can look like horizontal shimmer/flicker.
    spiCmd_(0x33);
    spiDataBuf_((const uint8_t[]){0x00, 0x00, 0x02, 0x80, 0x00, 0x00}, 6);
    spiCmd_(0x37);
    spiDataBuf_((const uint8_t[]){0x00, 0x00}, 2);

    // CS stays low until RGB panel begin() completes.
  }
};

#endif // ST7701_28B
#endif // ST7701_DISPLAY_H
