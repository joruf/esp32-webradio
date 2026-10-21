

/**
 * @desc	Queue a special function for the play task.
 * */
void queueFunctionForPlayTask(int func) {
  if (!vs1053Available) {
    return;
  }

  qdata_struct specchunk; // Special function to queue

  specchunk.datatyp = func;               // Put function in datatyp
  xQueueSend(dataqueue, &specchunk, 200); // Send to queue
}

/**
 * @desc	Check if a line in the header is a reasonable headerline.
 * 			Normally it should contain something like
 * "icy-xxxx:abcdef".
 * */
bool checkHeaderLine(const char *str) {
  char b;      // Byte examined
  int len = 0; // Lengte van de string

  while ((b = *str++)) {                    // Search to end of string
    len++;                                  // Update string length
    if (!isalpha(b)) {                      // Alpha (a-z, A-Z)
      if (b != '-') {                       // Minus sign is allowed
        if (b == ':') {                     // Found a colon
          return ((len > 5) && (len < 50)); // if length is okay
        } else {
          return false; // Not a legal character
        }
      }
    }
  }

  return false; // End of string without colon
}

/**
 * @brief Check whether host points to a finite MP3 file URL.
 *
 * @param streamHost Stream host/path without scheme
 * @return true if URL looks like a single .mp3 file
 */
static bool isFiniteMp3FileUrl(const String &streamHost) {
  String normalized = streamHost;
  int inx;

  normalized.trim();
  normalized.toLowerCase();

  inx = normalized.indexOf("?");
  if (inx >= 0) {
    normalized = normalized.substring(0, inx);
  }

  inx = normalized.indexOf("#");
  if (inx >= 0) {
    normalized = normalized.substring(0, inx);
  }

  return normalized.endsWith(".mp3");
}

#include "audio_stream_parser.h"
#if defined(ST7701_28B)
#include <esp_task_wdt.h>
#include "boot/st7701_boot_wdt.h"
#include "system/event_log.h"
#endif
#if defined(RADIO_I2S_A2DP_TX)
#include "audio/radio_i2s_a2dp.h"
#endif

/**
 * @desc	Called from the main loop() for the mp3 functions.
 */
void mp3loop() {
  uint32_t maxchunk; // Max number of bytes to read
  int res = 0;       // Result reading from mp3 stream
  uint32_t av = 0;   // Available in stream
  String nodeID;     // Next nodeID of track on SD
  uint32_t timing;   // Startime and duration this function
  uint32_t qspace;   // Free space in data queue

#if defined(RADIO_I2S_A2DP_TX)
  /* Soft-decode path: skip mp3client -> VS1053 pump; handle preset/host below. */
#else
  if (datamode & (INIT | HEADER | DATA | METADATA | PLAYLISTINIT | PLAYLISTHEADER | PLAYLISTDATA)) {
    timing = millis();
    maxchunk = sizeof(tmpbuff);
    if (vs1053Available) {
      qspace = uxQueueSpacesAvailable(dataqueue) * sizeof(qdata_struct);
    } else {
      qspace = maxchunk;
    }

    if (localfile) {
      av = mp3filelength;

      if (av < maxchunk) {
        maxchunk = av;
      }

      if (maxchunk > qspace) {
        maxchunk = qspace;
      }

      if (maxchunk) {
        if (!claimSPI("sdread")) {
          return;
        }
        res = mp3file.read(tmpbuff, maxchunk);
        releaseSPI();
        mp3filelength -= res;
      }
    } else {
      av = mp3client.available();

      if (av < maxchunk) {
        maxchunk = av;
      }

      if (maxchunk > qspace) {
        maxchunk = qspace;
      }

      if (maxchunk) {
        res = mp3client.read(tmpbuff, maxchunk);
        streamWatchdogFeed((uint16_t)res);
        vuMeterFeed((uint16_t)res);
      } else if (!localfile && mp3client.connected() && (datamode == DATA || datamode == PLAYLISTDATA)) {
        if (mp3client.available() > 0) {
          streamWatchdogFeed(1);
        }
      } else if (datamode == PLAYLISTDATA) {
        playlist_num = 0;
        dbgprint("End of playlist seen");
        datamode = STOPPED;
        ini_block.newpreset++;
      } else if (!mp3client.connected() && isFiniteMp3FileUrl(host)) {
        dbgprint("End of finite MP3 stream detected, stopping playback");
        hostreq = false;
        datamode = STOPREQD;
      }
    }

    for (int i = 0; i < res; i++) {
      handleNextDataByteFromServer(tmpbuff[i]);
    }

    timing = millis() - timing;
    if (timing > max_mp3loop_time) {
      max_mp3loop_time = timing;
      dbgprint("Duration mp3loop %d", timing);
    }
  }

  if (datamode == STOPREQD) {
    dbgprint("STOP requested");
    if (localfile) {
      if (claimSPI("close")) {
        mp3file.close();
        releaseSPI();
      }
    } else {
      stop_mp3client();
    }

    chunked = false;
    datacount = 0;
    outqp = outchunk.buf;
    queueFunctionForPlayTask(QSTOPSONG);
    metaint = 0;
    datamode = STOPPED;
    mbitrate = 0;
    bitrate = 0;
    return;
  }

  if (localfile) {
    if (datamode & DATA) {
      if (av == 0) {
        datamode = STOPREQD;
        nodeID = selectnextSDnode(SD_currentnode, +1);
        host = getSDfilename(nodeID);
        hostreq = true;
      }
    }
  }
#endif /* !RADIO_I2S_A2DP_TX */

  if (ini_block.newpreset != currentpreset) {
    if (datamode != STOPPED) {
      datamode = STOPREQD;
    } else {
      if (playlist_num) {
        playlist_num += ini_block.newpreset - currentpreset;
        ini_block.newpreset = currentpreset;
      } else {
        host = nvsGetHostFromPref();
        chomp(host);
      }

      dbgprint("New preset/file requested (%d/%d) from %s", ini_block.newpreset, playlist_num, host.c_str());

      if (host != "") {
        hostreq = true;
      } else {
        dbgprint("No host for this preset");
        ini_block.newpreset = 0;
      }
    }
  }

  if (hostreq) {
    hostreq = false;
    currentpreset = ini_block.newpreset;
    localfile = (host.indexOf("localhost/") >= 0);
    if (localfile) {
      if (connecttofile()) {
        datamode = DATA;
      }
    } else {
      if (host.startsWith("ihr/")) {
        host = host.substring(4);
        host = xmlgethost(host);
      }

#if defined(RADIO_I2S_A2DP_TX)
      /* Soft-decode to I2S for WROOM A2DP; skip VS1053 HTTP pump. */
      if (radioI2sA2dpConnect(host)) {
        dbgprint("RADIO_I2S_A2DP: streaming via I2S slave TX (WROOM master)");
      }
#else
      connectToWebradioServer();
#endif
    }
  }

#if defined(RADIO_I2S_A2DP_TX)
  if (datamode == STOPREQD) {
    radioI2sA2dpStop();
    datamode = STOPPED;
  }
#endif
}

/**
 * @desc	Disconnect from the server
 * */
void stop_mp3client() {
  // NOTE: This now runs inside the dedicated stream producer task. The previous
  // implementation used delay(500) inside a while(connected()) loop, which could
  // block the producer for several seconds on every reconnect and starve the
  // audio queue. Close the socket and wait only briefly (bounded) for it to drop.
  mp3client.flush();
  mp3client.stop();
  uint8_t retries = 0;
  while (mp3client.connected() && retries < 10) {
    mp3client.stop();
    vTaskDelay(pdMS_TO_TICKS(5)); // Yield without a long blocking delay
    retries++;
  }
  mp3client.flush(); // Flush stream client
  mp3client.stop();  // Stop stream client
}

/**
 * @brief Dedicated stream producer task.
 *
 * Runs the network/SD to audio-queue pump (mp3loop) and the stall watchdog on a
 * high-priority task so buffering the stream always takes precedence over the
 * HTTP server, web UI, SSE, encoder and other work in loop(). This prevents
 * audio dropouts when the web interface is busy.
 *
 * The pump is paused while an OTA/firmware update is running because the update
 * reuses the same WiFi client (mp3client) as the stream.
 *
 * @param parameter Unused FreeRTOS task parameter
 */
void mp3StreamTask(void *parameter) {
#if defined(ST7701_28B)
  esp_task_wdt_add(NULL);
#endif
  while (true) {
#if defined(ST7701_28B)
    esp_task_wdt_reset();
#if !defined(RADIO_I2S_A2DP_TX)
    if (decoderStallRecoverReq) {
      decoderStallRecoverReq = false;
      dbgprint("Decoder stall recovery (soft stop/reconnect, no reboot)");
      eventLogRecordWarning("Decoder stall recovery");
      if (datamode != STOPPED) {
        datamode = STOPREQD;
      }
    }
#else
    decoderStallRecoverReq = false; /* I2S soft-decode path owns stall handling */
#endif
#endif
    if (otaActive) {
      vTaskDelay(50 / portTICK_PERIOD_MS);
      continue;
    }

    mp3loop(); // Preset/host state machine (+ VS1053 pump when not I2S bridge)
#if !defined(RADIO_I2S_A2DP_TX)
    streamWatchdogPoll(); // Handle stalled-stream detection in the same task
#endif
    // Only touch SPI/VS1053 sine path while an offline wake tone is active.
    if (isAlarmFallbackPlaying()) {
      pollAlarmFallback();
    }

    // Yield ~1 ms so loop() (HTTP/UI) and other tasks still get CPU time.
    vTaskDelay(1);
  }
}
