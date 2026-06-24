/**
 * @file stream_watchdog.h
 * @brief Detect stalled streams and trigger automatic reconnect.
 */

#ifndef STREAM_WATCHDOG_H
#define STREAM_WATCHDOG_H

#include "features_config.h"

/**
 * @brief Detects stalled network streams and triggers reconnect.
 */
class StreamWatchdog {
public:
  /**
   * @brief Notify the watchdog that fresh stream data arrived.
   *
   * @param bytes Number of bytes read from the stream
   */
  inline void feed(uint16_t bytes) {
    if (bytes > 0) {
      lastDataMs_ = millis();
      stallMs_ = 0;
      reconnectAttempts_ = 0;
    }
  }

  /**
   * @brief Milliseconds since the last stream data chunk.
   *
   * @return uint32_t Stall duration in ms
   */
  uint32_t stallMs(void) const {
    if (lastDataMs_ == 0 || datamode == STOPPED) {
      return 0;
    }
    return millis() - lastDataMs_;
  }

  /**
   * @brief Poll stream health and reconnect when stalled.
   */
  void poll(void) {
    if (!features.stream_watchdog || localfile || datamode == STOPPED) {
      lastDataMs_ = 0;
      return;
    }

    if (lastDataMs_ == 0) {
      lastDataMs_ = millis();
      return;
    }

    stallMs_ = millis() - lastDataMs_;
    if (stallMs_ < 30000) {
      return;
    }

    if (reconnectAttempts_ >= 5) {
      return;
    }

    reconnectAttempts_++;
    dbgprint("Stream watchdog: reconnect attempt %d after %lu ms stall", reconnectAttempts_,
             (unsigned long)stallMs_);
    lastDataMs_ = millis();
    stop_mp3client();
    datamode = STOPREQD;
    hostreq = true;
  }

private:
  uint32_t lastDataMs_ = 0;
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
