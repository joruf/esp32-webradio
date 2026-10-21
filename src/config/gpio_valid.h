#ifndef GPIO_VALID_H
#define GPIO_VALID_H

#include <stdint.h>
#include "audio/mcp23017_vs1053.h"

/**
 * @brief True when pin may be used with pinMode/digitalRead on this chip.
 *
 * ESP32-S3 exposes GPIO 0-21 and 26-48; GPIO 22-25 do not exist (unlike classic ESP32).
 */
inline bool gpioIsValid(int8_t pin) {
  if (pin < 0) {
    return false;
  }
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  if (pin <= 21) {
    return true;
  }
  if (pin >= 26 && pin <= 48) {
    return true;
  }
  return false;
#else
  return pin <= 39;
#endif
}

/**
 * @brief True when VS1053 may use the shared SPI bus (all mandatory signals assigned and safe).
 *
 * ST7701 2.8B prefers avoiding USB D-/D+ (19/20). MISO may be -1 (write-only SCI).
 * GPIO16 may be DREQ after GT911 address-select reset (touch then polls over I2C only).
 * CS/DCS may be MCP sentinels (-20/-21).
 */
inline bool vs1053SpiBusConfigured(int8_t csPin, int8_t dcsPin, int8_t dreqPin, int8_t sckPin, int8_t misoPin,
                                   int8_t mosiPin) {
  if (dreqPin < 0 || sckPin < 0 || mosiPin < 0) {
    return false;
  }
  const bool mcpCs = vs1053UsesMcpCsDcs(csPin, dcsPin);
  if (!mcpCs) {
    if (csPin < 0 || dcsPin < 0) {
      return false;
    }
    if (!gpioIsValid(csPin) || !gpioIsValid(dcsPin)) {
      return false;
    }
  }
  if (!gpioIsValid(dreqPin) || !gpioIsValid(sckPin) || !gpioIsValid(mosiPin)) {
    return false;
  }
  if (misoPin >= 0 && !gpioIsValid(misoPin)) {
    return false;
  }
#if defined(ST7701_28B)
  // GPIO4: onboard battery ADC divider - unsafe for CS/DCS/DREQ.
  if (csPin == 4 || dcsPin == 4 || dreqPin == 4 || misoPin == 4 || sckPin == 4 || mosiPin == 4) {
    return false;
  }
  // USB D- (19): never use (MISO fought USB). D+ (20) allowed only as DREQ.
  if (csPin == 19 || csPin == 20 || dcsPin == 19 || dcsPin == 20 || sckPin == 19 || sckPin == 20 ||
      mosiPin == 19 || mosiPin == 20 || misoPin == 19 || misoPin == 20 || dreqPin == 19) {
    return false;
  }
#endif
  (void)mcpCs;
  return true;
}

#endif
