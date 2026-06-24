/**
 * @file stream_watchdog.h
 * @brief Detect stalled streams and trigger automatic reconnect.
 */

#ifndef STREAM_WATCHDOG_H
#define STREAM_WATCHDOG_H

#include "features_config.h"

static uint32_t streamLastDataMs = 0;
static uint32_t streamStallMs = 0;
static uint8_t streamReconnectAttempts = 0;

/**
 * @brief Notify the watchdog that fresh stream data arrived.
 *
 * @param bytes Number of bytes read from the stream
 */
void streamWatchdogFeed(uint16_t bytes) {
  if (bytes > 0) {
    streamLastDataMs = millis();
    streamStallMs = 0;
    streamReconnectAttempts = 0;
  }
}

/**
 * @brief Milliseconds since the last stream data chunk.
 *
 * @return uint32_t Stall duration in ms
 */
uint32_t getStreamStallMs(void) {
  if (streamLastDataMs == 0 || datamode == STOPPED) {
    return 0;
  }
  return millis() - streamLastDataMs;
}

/**
 * @brief Poll stream health and reconnect when stalled.
 */
void streamWatchdogPoll(void) {
  if (!features.stream_watchdog || localfile || datamode == STOPPED) {
    streamLastDataMs = 0;
    return;
  }

  if (streamLastDataMs == 0) {
    streamLastDataMs = millis();
    return;
  }

  streamStallMs = millis() - streamLastDataMs;
  if (streamStallMs < 30000) {
    return;
  }

  if (streamReconnectAttempts >= 5) {
    return;
  }

  streamReconnectAttempts++;
  dbgprint("Stream watchdog: reconnect attempt %d after %lu ms stall", streamReconnectAttempts,
           (unsigned long)streamStallMs);
  streamLastDataMs = millis();
  stop_mp3client();
  datamode = STOPREQD;
  hostreq = true;
}

#endif
