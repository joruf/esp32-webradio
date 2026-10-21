#ifndef VS1053_FAULT_H
#define VS1053_FAULT_H

#include "config/nvs.h"
#include "globals/globals.h"
#include "globals/app_prototypes.h"
#include "system/event_log.h"

#if defined(ST7701_28B)

/** Consecutive DREQ waits that ended in timeout before audio is disabled. */
static constexpr uint8_t VS1053_DREQ_FAULT_THRESHOLD = 24;

inline uint8_t &vs1053DreqTimeoutCounter(void) {
  static uint8_t count = 0;
  return count;
}

inline void vs1053ClearDreqTimeouts(void) {
  vs1053DreqTimeoutCounter() = 0;
}

inline void vs1053RecordDreqTimeout(void) {
  uint8_t &consecutive = vs1053DreqTimeoutCounter();
  if (++consecutive < VS1053_DREQ_FAULT_THRESHOLD) {
    return;
  }
  consecutive = 0;
  vs1053Available = false;
  eventLogRecordError("VS1053 DREQ timeout, audio path disabled (no reboot)");
  dbgprint("VS1053: disabling decoder after repeated DREQ timeouts");
}

inline void vs1053NoteSuccessfulFeed(void) {
  vs1053ClearDreqTimeouts();
}

#else

inline void vs1053ClearDreqTimeouts(void) {}
inline void vs1053RecordDreqTimeout(void) {}
inline void vs1053NoteSuccessfulFeed(void) {}

#endif

#endif
