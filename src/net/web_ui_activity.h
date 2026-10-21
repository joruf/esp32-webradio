#ifndef WEB_UI_ACTIVITY_H
#define WEB_UI_ACTIVITY_H

/**
 * @file web_ui_activity.h
 * @brief Timestamp for recent HTTP/Web UI traffic (TFT status icon).
 */

#include <Arduino.h>

static uint32_t webUiActivityLastMs = 0;

/** Call when the ESP handles HTTP from a browser or pushes SSE data. */
inline void webUiActivityMark(void) {
  webUiActivityLastMs = millis();
}

/** True for a short window after the last Web UI traffic. */
inline bool webUiActivityLive(void) {
  if (webUiActivityLastMs == 0) {
    return false;
  }
  return (millis() - webUiActivityLastMs) < 2000UL;
}

#endif
