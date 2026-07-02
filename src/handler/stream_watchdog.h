/**
 * @file stream_watchdog.h
 * @brief Detect stalled streams and trigger automatic reconnect.
 */

#ifndef STREAM_WATCHDOG_H
#define STREAM_WATCHDOG_H

#include "features_config.h"
#include "globals/globals.h"
#include "globals/app_prototypes.h"

static const uint32_t STREAM_WATCHDOG_STALL_MS = 60000UL;

/**
 * @brief Reconnect the current station without a full decoder stop/fade cycle.
 */
static void requestSoftStreamReconnect(void) {
  stop_mp3client();
  chunked = false;
  datacount = 0;
  outqp = outchunk.buf;
  metaint = 0;
  if (dataqueue) {
    xQueueReset(dataqueue);
  }
  if (host.length() > 0) {
    hostreq = true;
  }
}

/**
 * @brief Detects stalled network streams and triggers reconnect.
 */
class StreamWatchdog {
public:
  /**
   * @brief Notify the watchdog that stream activity was observed.
   *
   * @param bytes Number of bytes read from the network or sent to the decoder
   */
  inline void feed(uint16_t bytes) {
    if (bytes > 0) {
      lastActivityMs_ = millis();
      stallMs_ = 0;
      reconnectAttempts_ = 0;
    }
  }

  /**
   * @brief Milliseconds since the last stream activity.
   *
   * @return uint32_t Stall duration in ms
   */
  uint32_t stallMs(void) const {
    if (lastActivityMs_ == 0 || datamode == STOPPED) {
      return 0;
    }
    return millis() - lastActivityMs_;
  }

  /**
   * @brief Poll stream health and reconnect when stalled.
   */
  void poll(void) {
    if (!features.stream_watchdog || localfile || datamode == STOPPED) {
      lastActivityMs_ = 0;
      return;
    }

    if (lastActivityMs_ == 0) {
      lastActivityMs_ = millis();
      return;
    }

    stallMs_ = millis() - lastActivityMs_;
    if (stallMs_ < STREAM_WATCHDOG_STALL_MS) {
      return;
    }

    if (reconnectAttempts_ >= 5) {
      return;
    }

    reconnectAttempts_++;
    dbgprint("Stream watchdog: soft reconnect attempt %d after %lu ms stall", reconnectAttempts_,
             (unsigned long)stallMs_);
    lastActivityMs_ = millis();
    requestSoftStreamReconnect();
  }

private:
  uint32_t lastActivityMs_ = 0;
  uint32_t stallMs_ = 0;
  uint8_t reconnectAttempts_ = 0;
};

StreamWatchdog streamWatchdog;

inline void streamWatchdogFeed(uint16_t bytes) {
  streamWatchdog.feed(bytes);
}

inline uint32_t getStreamStallMs(void) {
  return streamWatchdog.stallMs();
}

inline void streamWatchdogPoll(void) {
  streamWatchdog.poll();
}

#endif
