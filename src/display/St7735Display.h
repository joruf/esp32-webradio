#ifndef ST7735_DISPLAY_H
#define ST7735_DISPLAY_H

#include <Adafruit_ST7735.h>

/**
 * @brief Thin wrapper around Adafruit ST7735 for the 160x128 color TFT.
 */
class St7735Display {
public:
  St7735Display() = default;

  /**
   * @brief Initialize the ST7735 panel using pins from ini_block.
   *
   * @return true when the panel object was created
   */
  bool begin() {
    panel_ = new Adafruit_ST7735(ini_block.tft_cs_pin, ini_block.tft_dc_pin, -1);
    if (!panel_) {
      return false;
    }
    panel_->initR(INITR_BLACKTAB);
    return true;
  }

  /**
   * @return Pointer to the underlying Adafruit driver, or nullptr
   */
  Adafruit_ST7735 *panel() {
    return panel_;
  }

  static constexpr uint16_t kWidth = 160;
  static constexpr uint16_t kHeight = 128;

private:
  Adafruit_ST7735 *panel_ = nullptr;
};

#endif
