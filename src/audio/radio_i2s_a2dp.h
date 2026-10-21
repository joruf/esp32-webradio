/**
 * @file radio_i2s_a2dp.h
 * @brief Soft-decode webradio and TX PCM as I2S master to WROOM (A2DP bridge).
 *
 * Enabled with -DRADIO_I2S_A2DP_TX. Keeps ST7701 + WiFi; skips VS1053 SPI on
 * GPIO43/44/16.
 *
 * Clocking: this board is I2S master TX (BCLK/LRCK/DOUT). WROOM is I2S slave
 * RX (legacy driver). ESP32-audioI2S is decode-only; PCM via audio_process_i2s().
 *
 * Wiring:
 *   S3 BCLK=43 -> WROOM BCLK=26
 *   S3 LRCK=44 -> WROOM LRCK=25
 *   S3 DOUT=16 -> WROOM DIN=32
 *   GND common
 */

#ifndef RADIO_I2S_A2DP_H
#define RADIO_I2S_A2DP_H

#if defined(RADIO_I2S_A2DP_TX)

#include <Arduino.h>

#pragma push_macro("sv")
#undef sv
#include <Audio.h>
#pragma pop_macro("sv")

#include <driver/i2s_std.h>
#include <esp_task_wdt.h>
#include <freertos/ringbuf.h>

#include "globals/app_prototypes.h"
#include "globals/globals.h"

#ifndef RADIO_I2S_BCLK
#define RADIO_I2S_BCLK 43
#endif
#ifndef RADIO_I2S_LRCK
#define RADIO_I2S_LRCK 44
#endif
#ifndef RADIO_I2S_DOUT
#define RADIO_I2S_DOUT 16
#endif

/* Dummy args for Audio::setPinout (decode-only patch ignores pins). */
#ifndef RADIO_I2S_DUMMY_BCLK
#define RADIO_I2S_DUMMY_BCLK 26
#endif
#ifndef RADIO_I2S_DUMMY_LRCK
#define RADIO_I2S_DUMMY_LRCK 27
#endif
#ifndef RADIO_I2S_DUMMY_DOUT
#define RADIO_I2S_DUMMY_DOUT 42
#endif

#ifndef RADIO_I2S_SAMPLE_RATE
#define RADIO_I2S_SAMPLE_RATE 44100
#endif

namespace radio_i2s_a2dp {
inline Audio gAudio;
inline bool gReady = false;
inline bool gPlaying = false;
inline bool gStopReq = false;
inline String gLastUrl;
inline char gPendingUrl[256] = {0};
inline SemaphoreHandle_t gMtx = nullptr;
inline uint32_t gConnectMs = 0;
inline uint32_t gLastStatusMs = 0;
inline TaskHandle_t gLoopTask = nullptr;
inline TaskHandle_t gTxTask = nullptr;
inline i2s_chan_handle_t gI2sTx = nullptr;
inline bool gI2sTxReady = false;
inline RingbufHandle_t gPcmRing = nullptr;
inline uint32_t gPcmPushed = 0;
inline uint32_t gPcmWritten = 0;

inline uint8_t mapVolume(uint8_t vol0to100) {
  if (vol0to100 > 100) {
    vol0to100 = 100;
  }
  return (uint8_t)((vol0to100 * 21U + 50U) / 100U);
}

inline void applyVolume(void) {
  if (!gReady) {
    return;
  }
  gAudio.setVolume(mapVolume(ini_block.reqvol));
}

inline String makeUrl(const String &hostIn) {
  String url = hostIn;
  url.trim();
  if (url.length() == 0) {
    return url;
  }
  if (url.startsWith("http://") || url.startsWith("https://")) {
    return url;
  }
  return String("http://") + url;
}

inline bool i2sInitSlaveTx() {
  if (gI2sTx) {
    i2s_channel_disable(gI2sTx);
    i2s_del_channel(gI2sTx);
    gI2sTx = nullptr;
  }
  gI2sTxReady = false;

  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_SLAVE);
  chan_cfg.auto_clear = true;
  chan_cfg.dma_desc_num = 8;
  chan_cfg.dma_frame_num = 256;
  if (i2s_new_channel(&chan_cfg, &gI2sTx, nullptr) != ESP_OK) {
    dbgprint("RADIO_I2S_A2DP: slave TX new_channel failed");
    return false;
  }

  i2s_std_clk_config_t clk = I2S_STD_CLK_DEFAULT_CONFIG(RADIO_I2S_SAMPLE_RATE);
  clk.bclk_div = 8;

  i2s_std_config_t std_cfg = {
      .clk_cfg = clk,
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                      I2S_SLOT_MODE_STEREO),
      .gpio_cfg =
          {
              .mclk = I2S_GPIO_UNUSED,
              .bclk = (gpio_num_t)RADIO_I2S_BCLK,
              .ws = (gpio_num_t)RADIO_I2S_LRCK,
              .dout = (gpio_num_t)RADIO_I2S_DOUT,
              .din = I2S_GPIO_UNUSED,
              .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false},
          },
  };

  if (i2s_channel_init_std_mode(gI2sTx, &std_cfg) != ESP_OK) {
    dbgprint("RADIO_I2S_A2DP: slave TX init_std failed");
    i2s_del_channel(gI2sTx);
    gI2sTx = nullptr;
    return false;
  }
  if (i2s_channel_enable(gI2sTx) != ESP_OK) {
    dbgprint("RADIO_I2S_A2DP: slave TX enable failed");
    i2s_del_channel(gI2sTx);
    gI2sTx = nullptr;
    return false;
  }
  gI2sTxReady = true;
  dbgprint("RADIO_I2S_A2DP: SLAVE TX BCLK=%d LRCK=%d DOUT=%d @ %d Hz", RADIO_I2S_BCLK,
           RADIO_I2S_LRCK, RADIO_I2S_DOUT, RADIO_I2S_SAMPLE_RATE);
  return true;
}

void creditDmaDesc(void);
void pushPcmWords(const int32_t *words, int16_t wordCount);

inline void txTask(void *) {
  for (;;) {
    if (!gI2sTxReady || !gI2sTx || !gPcmRing) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }
    size_t itemSize = 0;
    void *item = xRingbufferReceive(gPcmRing, &itemSize, pdMS_TO_TICKS(50));
    if (!item || itemSize == 0) {
      continue;
    }
    size_t written = 0;
    const esp_err_t err =
        i2s_channel_write(gI2sTx, item, itemSize, &written, pdMS_TO_TICKS(100));
    if (err == ESP_OK) {
      gPcmWritten += (uint32_t)(written / 4U);
    }
    vRingbufferReturnItem(gPcmRing, item);
  }
}

inline void connectLocked(const String &url) {
  dbgprint("RADIO_I2S_A2DP: connecttohost %s", url.c_str());
  gAudio.stopSong();
  const bool ok = gAudio.connecttohost(url.c_str());
  gLastUrl = url;
  gConnectMs = millis();
  gPlaying = ok;
  if (ok) {
    datamode = DATA;
    metaSetStation(url.c_str());
    applyVolume();
    dbgprint("RADIO_I2S_A2DP: connecttohost OK");
  } else {
    datamode = STOPPED;
    dbgprint("RADIO_I2S_A2DP: connecttohost FAILED");
  }
}

inline void loopTask(void *) {
  static uint8_t lastVol = 0xFF;
  for (;;) {
    char pending[sizeof(gPendingUrl)] = {0};
    bool stopReq = false;
    if (gMtx && xSemaphoreTake(gMtx, pdMS_TO_TICKS(5)) == pdTRUE) {
      if (gPendingUrl[0]) {
        strncpy(pending, gPendingUrl, sizeof(pending) - 1);
        gPendingUrl[0] = '\0';
      }
      stopReq = gStopReq;
      gStopReq = false;
      xSemaphoreGive(gMtx);
    }

    if (stopReq) {
      gAudio.stopSong();
      gPlaying = false;
      datamode = STOPPED;
    }
    if (pending[0]) {
      connectLocked(String(pending));
    }

    if (gReady) {
      gAudio.loop();
      if (gPlaying && gAudio.m_dmaFreeDesc.load() == 0) {
        gAudio.m_dmaFreeDesc.store(gAudio.settings.DMA_DESC_NUM);
      }
      if (gPlaying && gAudio.isRunning()) {
        totalcount += 64;
      }
      if (lastVol != ini_block.reqvol) {
        lastVol = ini_block.reqvol;
        applyVolume();
      }
      const uint32_t now = millis();
      if (now - gLastStatusMs >= 5000UL) {
        gLastStatusMs = now;
        dbgprint("RADIO_I2S_A2DP: running=%d playing=%d slaveTx=%d pushed=%lu written=%lu dma=%lu heap=%u",
                 (int)gAudio.isRunning(), (int)gPlaying, (int)gI2sTxReady,
                 (unsigned long)gPcmPushed, (unsigned long)gPcmWritten,
                 (unsigned long)gAudio.m_dmaFreeDesc.load(), (unsigned)ESP.getFreeHeap());
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
} // namespace radio_i2s_a2dp

inline bool radioI2sA2dpEnabled(void) { return true; }

inline bool radioI2sA2dpBegin(void) {
  using namespace radio_i2s_a2dp;
  if (gReady) {
    return true;
  }

  ini_block.spi_sck_pin = -1;
  ini_block.spi_mosi_pin = -1;
  ini_block.spi_miso_pin = -1;
  ini_block.vs_cs_pin = -1;
  ini_block.vs_dcs_pin = -1;
  ini_block.vs_dreq_pin = -1;
  vs1053Available = false;

  if (gMtx == nullptr) {
    gMtx = xSemaphoreCreateMutex();
  }
  if (gPcmRing == nullptr) {
    gPcmRing = xRingbufferCreate(44100 / 10 * 4, RINGBUF_TYPE_BYTEBUF);
  }
  if (!gPcmRing) {
    dbgprint("RADIO_I2S_A2DP: PCM ring alloc failed");
    return false;
  }

  (void)esp_task_wdt_reset();
  if (!gAudio.setPinout(RADIO_I2S_DUMMY_BCLK, RADIO_I2S_DUMMY_LRCK, RADIO_I2S_DUMMY_DOUT)) {
    dbgprint("RADIO_I2S_A2DP: Audio setPinout FAILED");
    return false;
  }
  gAudio.m_dmaFreeDesc.store(gAudio.settings.DMA_DESC_NUM);
  dbgprint("RADIO_I2S_A2DP: Audio decode-only ready; wire PCM via I2S slave TX");
  gAudio.setVolume(mapVolume(ini_block.reqvol));

  if (!i2sInitSlaveTx()) {
    return false;
  }

  gReady = true;
  if (gTxTask == nullptr) {
    xTaskCreatePinnedToCore(txTask, "radioI2sTx", 4096, nullptr, 5, &gTxTask, 1);
  }
  if (gLoopTask == nullptr) {
    xTaskCreatePinnedToCore(loopTask, "radioI2sLoop", 12288, nullptr, 4, &gLoopTask, 1);
  }
  dbgprint("RADIO_I2S_A2DP: decode+slaveTX ready; WROOM must be I2S master RX");
  dbgprint("RADIO_I2S_A2DP: USB CDC serial only (GPIO43/44 = I2S)");
  return true;
}

inline bool radioI2sA2dpConnect(const String &hostIn) {
  using namespace radio_i2s_a2dp;
  if (!gReady && !radioI2sA2dpBegin()) {
    return false;
  }
  const String url = makeUrl(hostIn);
  if (url.length() == 0) {
    return false;
  }
  if (gMtx && xSemaphoreTake(gMtx, pdMS_TO_TICKS(100)) == pdTRUE) {
    strncpy(gPendingUrl, url.c_str(), sizeof(gPendingUrl) - 1);
    gPendingUrl[sizeof(gPendingUrl) - 1] = '\0';
    gStopReq = false;
    xSemaphoreGive(gMtx);
  }
  gPlaying = true;
  datamode = DATA;
  return true;
}

inline void radioI2sA2dpStop(void) {
  using namespace radio_i2s_a2dp;
  if (!gReady) {
    return;
  }
  if (gMtx && xSemaphoreTake(gMtx, pdMS_TO_TICKS(100)) == pdTRUE) {
    gPendingUrl[0] = '\0';
    gStopReq = true;
    xSemaphoreGive(gMtx);
  }
}

inline bool radioI2sA2dpIsPlaying(void) {
  using namespace radio_i2s_a2dp;
  return gReady && gPlaying && gAudio.isRunning();
}

inline void radioI2sA2dpLoop(void) {}

#else /* !RADIO_I2S_A2DP_TX */

inline bool radioI2sA2dpEnabled(void) { return false; }
inline bool radioI2sA2dpBegin(void) { return false; }
inline bool radioI2sA2dpConnect(const String &) { return false; }
inline void radioI2sA2dpStop(void) {}
inline void radioI2sA2dpLoop(void) {}
inline bool radioI2sA2dpIsPlaying(void) { return false; }

#endif /* RADIO_I2S_A2DP_TX */

#endif /* RADIO_I2S_A2DP_H */
