#ifndef ST7735_DISPLAY_H
#define ST7735_DISPLAY_H

#include <Adafruit_ST7735.h>

/**
 * @brief Adafruit ST7735 with accessible GRAM col/row start offsets.
 *
 * Cheap 160x128 clones often need a non-zero _rowstart in landscape so the
 * leftmost column is visible and no stray column appears on the right.
 * setColRowStart() is protected on Adafruit_ST77xx, so we expose it here.
 */
class RadioSt7735 : public Adafruit_ST7735 {
public:
  using Adafruit_ST7735::Adafruit_ST7735;

  /**
   * @brief Set panel GRAM origin and refresh _xstart/_ystart for the current rotation.
   *
   * @param colstart Portrait X / landscape Y offset into GRAM
   * @param rowstart Portrait Y / landscape X offset into GRAM
   */
  void applyPanelOffset(uint8_t colstart, uint8_t rowstart) {
    setColRowStart((int8_t)colstart, (int8_t)rowstart);
    setRotation(getRotation());
  }
};

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
    panel_ = new RadioSt7735(ini_block.tft_cs_pin, ini_block.tft_dc_pin, -1);
    if (!panel_) {
      return false;
    }
    // BLACKTAB keeps RGB MADCTL; per-device GRAM offsets are applied later via
    // applyPanelOffset() so green-tab clones can be corrected without BGR swap.
    panel_->initR(INITR_BLACKTAB);
    return true;
  }

  /**
   * @return Pointer to the underlying Adafruit driver, or nullptr
   */
  RadioSt7735 *panel() {
    return panel_;
  }

  /**
   * @brief Apply configurable GRAM col/row start for the active panel.
   */
  void applyPanelOffset(uint8_t colstart, uint8_t rowstart) {
    if (panel_) {
      panel_->applyPanelOffset(colstart, rowstart);
    }
  }

  static constexpr uint16_t kWidth = 160;
  static constexpr uint16_t kHeight = 128;

private:
  RadioSt7735 *panel_ = nullptr;
};

#endif
