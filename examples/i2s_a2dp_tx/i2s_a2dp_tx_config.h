/**
 * @file i2s_a2dp_tx_config.h
 * @brief ESP32-S3 I2S master TX pin map for the A2DP bridge to WROOM.
 *
 * Wiring (GND common; do not parallel 5V USB between boards):
 *   S3 BCLK GPIO43 (TXD)  -> WROOM GPIO26
 *   S3 LRCK GPIO44 (RXD)  -> WROOM GPIO25
 *   S3 DOUT GPIO16         -> WROOM GPIO32 (DIN)
 *
 * GPIO43/44 are UART0 TX/RX. This firmware uses USB CDC for serial logs
 * (ARDUINO_USB_CDC_ON_BOOT). Do not rely on UART0 for the monitor.
 *
 * These pins conflict with the webradio VS1053 SPI map (SCK/MOSI on 43/44,
 * DREQ/DCS often on 16). Free the headers for the I2S bridge test.
 */

#pragma once

#ifndef I2S_TX_SAMPLE_RATE
#define I2S_TX_SAMPLE_RATE 44100
#endif

#ifndef I2S_TX_TONE_HZ
/* 880 Hz so it is clearly distinct from the WROOM local 440 Hz fallback. */
#define I2S_TX_TONE_HZ 880
#endif

#ifndef I2S_TX_PIN_BCLK
#define I2S_TX_PIN_BCLK 43
#endif
#ifndef I2S_TX_PIN_LRCK
#define I2S_TX_PIN_LRCK 44
#endif
#ifndef I2S_TX_PIN_DOUT
#define I2S_TX_PIN_DOUT 16
#endif

#ifndef I2S_TX_AMPLITUDE
#define I2S_TX_AMPLITUDE 12000
#endif
