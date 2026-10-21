#ifndef VS1053_DEFERRED_H
#define VS1053_DEFERRED_H

#if defined(ST7701_28B)

#include <SPI.h>

#include "audio/mcp23017_vs1053.h"
#include "boot/st7701_boot_wdt.h"
#include "boot/st7701_usb_vs1053.h"
#include "config/gpio_valid.h"
#include "globals/globals.h"
#include "globals/app_prototypes.h"
#include "system/event_log.h"

namespace st7701_vs_deferred {
inline uint8_t gPhase = 0; // 0=wait, 1=run once, 2=done
inline uint32_t gStartAfterMs = 0;
inline char gFailReason[96] = "";
} // namespace st7701_vs_deferred

/** Last deferred-init failure text for diagnostics (empty on success). */
inline const char *st7701Vs1053FailReason(void) { return st7701_vs_deferred::gFailReason; }

/** Request another deferred VS1053 bring-up (e.g. after plugging MCP/VS1053). */
inline void st7701RequestVs1053Reinit(void) {
  st7701_vs_deferred::gPhase = 0;
  st7701_vs_deferred::gStartAfterMs = 0;
  st7701_vs_deferred::gFailReason[0] = '\0';
  vs1053Available = false;
}

/**
 * @brief Run once after WiFi/HTTP are up: init VS1053 with WDT feeds, start audio task on success.
 * @return true while still waiting (caller may yield)
 */
inline bool st7701PollVs1053DeferredInit(void) {
  using namespace st7701_vs_deferred;

#if defined(RADIO_I2S_A2DP_TX)
  /* I2S A2DP bridge owns GPIO43/44/16; never claim them for VS1053 SPI. */
  gPhase = 2;
  vs1053Available = false;
  return false;
#endif

  if (gPhase >= 2) {
    return false;
  }

  if (gPhase == 0) {
    if (gStartAfterMs == 0) {
      gStartAfterMs = millis() + 600UL;
    }
    if ((int32_t)(millis() - gStartAfterMs) < 0) {
      st7701FeedTaskWdt();
      return true;
    }
    gPhase = 1;
  }

  st7701FeedTaskWdt();
  gFailReason[0] = '\0';
  if (!vs1053player) {
    vs1053Available = false;
    gPhase = 2;
    strncpy(gFailReason, "no driver instance", sizeof(gFailReason) - 1);
    dbgprint("VS1053 deferred init skipped (no driver instance)");
    return false;
  }

  // Re-apply USB-free pin map (NVS may still hold old 19/20 values).
  ini_block.enc_clk_pin = -1;
  ini_block.enc_dt_pin = -1;
  ini_block.enc_sw_pin = -1;
  ini_block.spi_sck_pin = 43;
  ini_block.spi_mosi_pin = 44;
#if defined(USE_MCP23017_VS1053)
  if (mcp23017Vs1053Ready() || mcp23017Vs1053Begin()) {
    mcp23017Vs1053DeselectAll();
    ini_block.vs_cs_pin = VS1053_MCP_CS_SENTINEL;
    ini_block.vs_dcs_pin = VS1053_MCP_DCS_SENTINEL;
    ini_block.vs_dreq_pin = 16;
    ini_block.spi_miso_pin = 0;
    vs1053player->setPins(VS1053_MCP_CS_SENTINEL, VS1053_MCP_DCS_SENTINEL, 16);
  } else {
    ini_block.vs_cs_pin = 0;
    ini_block.vs_dcs_pin = 16;
    ini_block.vs_dreq_pin = 20;
    ini_block.spi_miso_pin = -1;
    vs1053player->setPins(0, 16, 20);
  }
#endif

  if (!vs1053SpiBusConfigured(ini_block.vs_cs_pin, ini_block.vs_dcs_pin, ini_block.vs_dreq_pin, ini_block.spi_sck_pin,
                              ini_block.spi_miso_pin, ini_block.spi_mosi_pin)) {
    vs1053Available = false;
    gPhase = 2;
    strncpy(gFailReason, "SPI pins not configured", sizeof(gFailReason) - 1);
    dbgprint("VS1053 deferred init skipped (SPI pins not configured)");
    return false;
  }

  // Only needed if someone still has wires on D-/D+; harmless otherwise.
  st7701ReleaseUsbPadsForVs1053();
  st7701FeedTaskWdt();

  const int8_t spiMiso =
      (ini_block.spi_miso_pin >= 0) ? ini_block.spi_miso_pin : (int8_t)(-1);
  SPI.begin(ini_block.spi_sck_pin, spiMiso, ini_block.spi_mosi_pin, -1);

  if (!vs1053UsesMcpCsDcs(ini_block.vs_cs_pin, ini_block.vs_dcs_pin)) {
    detachInterrupt(0);
    detachInterrupt(4);
    pinMode(ini_block.vs_cs_pin, OUTPUT);
    digitalWrite(ini_block.vs_cs_pin, HIGH);
    pinMode(ini_block.vs_dcs_pin, OUTPUT);
    digitalWrite(ini_block.vs_dcs_pin, HIGH);
  }
  pinMode(ini_block.vs_dreq_pin, INPUT);
  st7701FeedTaskWdt();

  dbgprint("BOOT: Deferred VS1053 begin CS=%d DCS=%d DREQ=%d SCK=%d MISO=%d MOSI=%d",
           (int)ini_block.vs_cs_pin, (int)ini_block.vs_dcs_pin, (int)ini_block.vs_dreq_pin,
           (int)ini_block.spi_sck_pin, (int)ini_block.spi_miso_pin, (int)ini_block.spi_mosi_pin);
  vs1053player->begin();
  st7701FeedTaskWdt();

  if (vs1053player->isReady()) {
    if (ini_block.spi_miso_pin < 0) {
      dbgprint("VS1053 init OK (write-only, DREQ=%d)", (int)ini_block.vs_dreq_pin);
      vs1053Available = true;
      gFailReason[0] = '\0';
    } else {
      uint16_t modeReg = vs1053player->read_register(0x00);
      st7701FeedTaskWdt();
      if (modeReg == 0x0000 || modeReg == 0xFFFF) {
        dbgprint("VS1053 not properly installed! Register-Dump: 0x%04X", modeReg);
        eventLogRecordError("VS1053 invalid mode register (check wiring)");
        snprintf(gFailReason, sizeof(gFailReason), "bad mode reg 0x%04X", modeReg);
        vs1053Available = false;
      } else {
        dbgprint("VS1053 init OK (deferred). Mode-Register: 0x%04X", modeReg);
        vs1053Available = true;
        gFailReason[0] = '\0';
      }
    }
    if (vs1053Available) {
      vs1053player->setVolume(0);
      vs1053player->setVolume(getChipVolume());
      vs1053player->setOutputEnabled(true);
    }
  } else {
    dbgprint("VS1053 not detected (deferred), continuing without audio decoder");
    eventLogRecordError("VS1053 audio decoder not detected");
    const int dreqLvl =
        (ini_block.vs_dreq_pin >= 0) ? (digitalRead(ini_block.vs_dreq_pin) ? 1 : 0) : -1;
    snprintf(gFailReason, sizeof(gFailReason), "init fail CS=%d DCS=%d DREQ=%d lvl=%d MISO=%d",
             (int)ini_block.vs_cs_pin, (int)ini_block.vs_dcs_pin, (int)ini_block.vs_dreq_pin, dreqLvl,
             (int)ini_block.spi_miso_pin);
    vs1053Available = false;
  }

  if (vs1053Available && xplayStreamFromQueue == NULL) {
    dbgprint("VS1053 available. Start Audio-Task (deferred)...");
    xTaskCreatePinnedToCore(playStreamFromQueue, "playStreamFromQueue", 4096, NULL, 3, &xplayStreamFromQueue, 0);
  }

  gPhase = 2;
  return false;
}

#endif // ST7701_28B

#endif // VS1053_DEFERRED_H
