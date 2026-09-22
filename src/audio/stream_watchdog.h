/**
 * @file stream_watchdog.h
 * @brief Detect stalled streams and trigger automatic reconnect.
 */

#ifndef STREAM_WATCHDOG_H
#define STREAM_WATCHDOG_H

#include "config/features_config.h"
#include "globals/globals.h"
#include "globals/app_prototypes.h"
#include "audio/alarm_fallback.h"

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
  // Keep any queued decoder chunks so playback can continue while the producer
  // reconnects. Wiping the queue caused an immediate audible gap on every stall
  // recovery (up to ~1.6 s of buffer at QSIZ=800).
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
    if (isAlarmFallbackPlaying()) {
      return;
    }

    // During an alarm session always watch for dead streams so offline tone can start,
    // even if the stream_watchdog feature toggle is off.
    if ((!radioFeatures.stream_watchdog && !isAlarmSessionActive()) || localfile || datamode == STOPPED) {
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
      if (!gaveUpLogged_) {
        gaveUpLogged_ = true;
        eventLogRecordError("Stream watchdog gave up after 5 reconnects");
        if (isAlarmSessionActive()) {
          startAlarmFallback();
        }
      }
      return;
    }

    reconnectAttempts_++;
    gaveUpLogged_ = false;
    dbgprint("Stream watchdog: soft reconnect attempt %d after %lu ms stall", reconnectAttempts_,
             (unsigned long)stallMs_);
    eventLogRecordWarning("Stream stalled, reconnecting");
    lastActivityMs_ = millis();
    requestSoftStreamReconnect();
  }

  /** Reset reconnect bookkeeping when a new alarm/stream attempt begins. */
  void reset(void) {
    lastActivityMs_ = 0;
    stallMs_ = 0;
    reconnectAttempts_ = 0;
    gaveUpLogged_ = false;
  }

private:
  uint32_t lastActivityMs_ = 0;
  uint32_t stallMs_ = 0;
  uint8_t reconnectAttempts_ = 0;
  bool gaveUpLogged_ = false; // Ensures the "gave up" error is logged only once
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

inline void streamWatchdogReset(void) {
  streamWatchdog.reset();
}

#endif
