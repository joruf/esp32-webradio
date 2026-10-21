/**
 * @file radio_i2s_a2dp_callbacks.cpp
 * @brief ESP32-audioI2S hooks for RADIO_I2S_A2DP_TX (info + PCM tap).
 *
 * Intentionally does NOT include Audio.h: that header declares
 * audio_process_i2s() as weak, which would make this definition weak too
 * and the empty library stub would win at link time.
 */

#if defined(RADIO_I2S_A2DP_TX)

#include "audio/radio_i2s_a2dp_api.h"
#include "globals/app_prototypes.h"
#include "globals/globals.h"

void audio_info(const char *info) {
  if (info) {
    dbgprint("I2S-Audio: %s", info);
  }
}

void audio_showstreamtitle(const char *info) {
  if (info && info[0]) {
    metaSetTitle(info);
  }
}

void audio_showstation(const char *info) {
  if (info && info[0]) {
    metaSetStation(info);
  }
}

void audio_eof_stream(const char *info) {
  dbgprint("I2S-Audio: eof_stream %s", info ? info : "");
}

/**
 * Strong override of Audio's weak stub. Must not see the weak declaration.
 *
 * @param outBuff int32 words (stereo interleaved, sample in high 16 bits)
 * @param validSamples number of int32 words (not frames)
 */
void audio_process_i2s(int32_t *outBuff, int16_t validSamples, bool *continueI2S) {
  if (continueI2S) {
    *continueI2S = false;
  }
  if (outBuff && validSamples >= 2) {
    radio_i2s_a2dp::pushPcmWords(outBuff, validSamples);
  }
  radio_i2s_a2dp::creditDmaDesc();
}

#endif /* RADIO_I2S_A2DP_TX */
