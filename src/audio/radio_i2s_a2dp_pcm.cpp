/**
 * @file radio_i2s_a2dp_pcm.cpp
 * @brief PCM ring + DMA credit helpers for RADIO_I2S_A2DP_TX.
 */

#if defined(RADIO_I2S_A2DP_TX)

#include "globals/app_prototypes.h"
#include "audio/radio_i2s_a2dp.h"

namespace radio_i2s_a2dp {

void creditDmaDesc(void) {
  gAudio.m_dmaFreeDesc.fetch_add(1, std::memory_order_release);
}

void pushPcmWords(const int32_t *words, int16_t wordCount) {
  if (!gPcmRing || !words || wordCount < 2) {
    return;
  }
  /* Convert Audio int32 (sample in high 16) to int16 stereo frames. */
  static int16_t tmp[256];
  int16_t remaining = (int16_t)(wordCount / 2);
  int16_t offset = 0;
  while (remaining > 0) {
    const int16_t nFrames = (remaining > 128) ? 128 : remaining;
    for (int16_t i = 0; i < nFrames; ++i) {
      const int16_t src = (int16_t)((offset + i) * 2);
      tmp[2 * i] = (int16_t)(words[src] >> 16);
      tmp[2 * i + 1] = (int16_t)(words[src + 1] >> 16);
    }
    const size_t bytes = (size_t)nFrames * 4U;
    if (xRingbufferSend(gPcmRing, tmp, bytes, 0) == pdTRUE) {
      gPcmPushed += (uint32_t)nFrames;
    }
    offset = (int16_t)(offset + nFrames);
    remaining = (int16_t)(remaining - nFrames);
  }
}

} // namespace radio_i2s_a2dp

#endif /* RADIO_I2S_A2DP_TX */
