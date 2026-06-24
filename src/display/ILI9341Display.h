#ifndef ILI9341_DISPLAY_H
#define ILI9341_DISPLAY_H

#include <Adafruit_ILI9341.h>

/**
 * @brief Thin wrapper around Adafruit ILI9341 for the 320x240 color TFT.
 */
class ILI9341Display {
public:
  ILI9341Display() = default;

  /**
   * @brief Initialize the ILI9341 panel using pins from ini_block.
   *
   * @return true when the panel object was created
   */
  bool begin() {
    panel_ = new Adafruit_ILI9341(ini_block.tft_cs_pin, ini_block.tft_dc_pin);
    if (!panel_) {
      return false;
    }
    panel_->begin();
    return true;
  }

  /**
   * @return Pointer to the underlying Adafruit driver, or nullptr
   */
  Adafruit_ILI9341 *panel() {
    return panel_;
  }

  static constexpr uint16_t kWidth = 320;
  static constexpr uint16_t kHeight = 240;

private:
  Adafruit_ILI9341 *panel_ = nullptr;
};

#endif
