#ifndef ST7701_BOARD_EARLY_H
#define ST7701_BOARD_EARLY_H

/**
 * @file st7701_board_early.h
 * @brief Minimal Waveshare 2.8B bring-up without the GFX stack (buzzer off early).
 */

#if defined(ST7701_28B)

#include <Arduino.h>
#include <Wire.h>

#ifndef ST7701_TCA9554_ADDR
#define ST7701_TCA9554_ADDR 0x20
#endif

/** TCA9554 EXIO8 (bit 7): onboard piezo, active high. */
#ifndef ST7701_EXIO_BUZZER_BIT
#define ST7701_EXIO_BUZZER_BIT 7
#endif

/**
 * @brief Silence the onboard buzzer via TCA9554 (call as early as setup allows).
 *
 * Without this, a later tcaOut_=0xFF during display init drives EXIO8 high and the
 * piezo beeps until RGB bring-up finishes (or through WDT boot loops).
 */
inline void st7701SilenceOnboardBuzzer(void) {
  Wire.begin(15, 7);
  Wire.setClock(400000);
  Wire.setTimeOut(25);

  Wire.beginTransmission(ST7701_TCA9554_ADDR);
  Wire.write(0x03); // direction: all outputs
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    return;
  }

  uint8_t out = 0xFF;
  out &= (uint8_t)~(1u << ST7701_EXIO_BUZZER_BIT);
  Wire.beginTransmission(ST7701_TCA9554_ADDR);
  Wire.write(0x01); // output port
  Wire.write(out);
  Wire.endTransmission();
}

#endif // ST7701_28B
#endif // ST7701_BOARD_EARLY_H
