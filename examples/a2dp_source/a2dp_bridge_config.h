/**
 * @file a2dp_bridge_config.h
 * @brief Pin map and build-time options for ESP32-WROOM A2DP Source bridge.
 *
 * Architecture: S3 I2S master TX drives BCLK/LRCK/DOUT; WROOM I2S slave RX
 * (esp32s3_touch28b + RADIO_I2S_A2DP_TX). WROOM forwards via Classic Bluetooth
 * A2DP Source -> headphones/speaker (A2DP Sink).
 *
 * WROOM pins avoid strapping pins 0 / 2 / 12 / 15.
 */

#pragma once

#ifndef A2DP_SAMPLE_RATE
#define A2DP_SAMPLE_RATE 44100
#endif

#ifndef A2DP_VOLUME
#define A2DP_VOLUME 64
#endif

/* Empty string = accept first discovered A2DP Sink (see ssid callback).
 * Override at build time, e.g.:
 *   -DBT_SINK_NAME=\"My\\ Headphones\"
 */
#ifndef BT_SINK_NAME
#define BT_SINK_NAME ""
#endif

/* 1 = open I2S slave RX and prefer external PCM; silence if no I2S data.
 * 0 = always play local test tone (no I2S driver). Use Serial "tone" to force beep.
 */
#ifndef A2DP_USE_I2S_INPUT
#define A2DP_USE_I2S_INPUT 1
#endif

/* WROOM I2S slave RX (clocks from S3 master TX). */
#ifndef I2S_PIN_BCLK
#define I2S_PIN_BCLK 26
#endif
#ifndef I2S_PIN_LRCK
#define I2S_PIN_LRCK 25
#endif
#ifndef I2S_PIN_DIN
#define I2S_PIN_DIN 32
#endif

/* Matching S3 Touch LCD 2.8B header pins (I2S master TX).
 * Conflict with VS1053 SPI on TXD/RXD/16; RADIO_I2S_A2DP_TX disables that path.
 * GND common; do not cross-power 5V USB.
 *
 *   Signal   S3 (master)           WROOM (slave)
 *   ------   --------------------  -------------
 *   BCLK     TXD (GPIO43)    ->    GPIO26
 *   LRCK     RXD (GPIO44)    ->    GPIO25
 *   DATA     GPIO16          ->    GPIO32 (DIN)
 *   GND      GND             <->   GND
 */
#ifndef S3_I2S_PIN_BCLK_SUGGEST
#define S3_I2S_PIN_BCLK_SUGGEST 43
#endif
#ifndef S3_I2S_PIN_LRCK_SUGGEST
#define S3_I2S_PIN_LRCK_SUGGEST 44
#endif
#ifndef S3_I2S_PIN_DOUT_SUGGEST
#define S3_I2S_PIN_DOUT_SUGGEST 16
#endif

/* Test tone when I2S has no data (Hz). */
#ifndef A2DP_TONE_HZ
#define A2DP_TONE_HZ 440
#endif
