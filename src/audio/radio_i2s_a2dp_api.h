/**
 * @file radio_i2s_a2dp_api.h
 * @brief PCM tap API without including Audio.h (avoids weak-symbol infection).
 */
#pragma once

#include <stdint.h>

#if defined(RADIO_I2S_A2DP_TX)
namespace radio_i2s_a2dp {
void pushPcmWords(const int32_t *words, int16_t wordCount);
void creditDmaDesc(void);
} // namespace radio_i2s_a2dp
#endif
