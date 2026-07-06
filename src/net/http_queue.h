/**
 * @file http_queue.h
 * @brief HTTP client queue and Server-Sent Events (SSE) support.
 */

#ifndef HTTP_QUEUE_H
#define HTTP_QUEUE_H

#include "live_status.h"

#define HTTP_PENDING_MAX 8

bool sseStreamActive = false;
uint32_t sseLastPushMs = 0;
int8_t sseLastPlaybackState = -1;

static WiFiClient httpPending[HTTP_PENDING_MAX];
static uint8_t httpPendingCount = 0;
static WiFiClient sseClient;

/**
 * @brief Accept pending HTTP clients while the active slot is busy.
 *
 * @param server   HTTP server instance
 * @param cmdclient Active HTTP client reference
 * @param responseFlag True while a response is being prepared
 */
static void httpAcceptPending(WiFiServer &server, WiFiClient &cmdclient, bool responseFlag) {
  while (server.hasClient()) {
    WiFiClient incoming = server.available();
    if (!incoming) {
      break;
    }

    if (!cmdclient.connected() && !responseFlag) {
      cmdclient = incoming;
      continue;
    }

    if (httpPendingCount < HTTP_PENDING_MAX) {
      httpPending[httpPendingCount++] = incoming;
    } else {
      incoming.stop();
    }
  }
}

/**
 * @brief Promote the next queued client when the active connection is idle.
 *
 * @param cmdclient Active HTTP client reference
 * @param responseFlag Response-in-progress flag
 */
static void httpPromotePending(WiFiClient &cmdclient, bool &responseFlag) {
  if (cmdclient.connected() || responseFlag || httpPendingCount == 0) {
    return;
  }

  cmdclient = httpPending[0];
  for (uint8_t i = 1; i < httpPendingCount; i++) {
    httpPending[i - 1] = httpPending[i];
  }
  httpPendingCount--;
}

/**
 * @brief Begin an SSE response on the given client.
 *
 * @param client Connected HTTP client
 */
static void httpBeginSse(WiFiClient &client) {
  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: text/event-stream; charset=utf-8\r\n");
  client.print("Cache-Control: no-cache\r\n");
  client.print("Connection: keep-alive\r\n");
  client.print("Access-Control-Allow-Origin: *\r\n\r\n");
}

/**
 * @brief Attach the current client as the active SSE stream.
 *
 * Transfers the connection to a dedicated SSE client so the main HTTP slot
 * remains available for API requests.
 *
 * @param client Connected HTTP client (released after transfer)
 */
static void httpAttachSseClient(WiFiClient &client) {
  if (sseStreamActive && sseClient.connected()) {
    sseClient.stop();
  }

  sseClient = client;
  client = WiFiClient();
  sseClient.setNoDelay(true); // Push events promptly without waiting for Nagle
  httpBeginSse(sseClient);
  sseStreamActive = true;
  sseLastPushMs = 0;
  sseLastPlaybackState = -1;
}

/**
 * @brief Push a live JSON event to the active SSE client.
 *
 * Runs from the main loop(). To keep the loop responsive on a slow or dead peer,
 * the event is sent with a single non-blocking write() (bounded by the WiFiClient
 * retry/select loop) and the previous per-fragment writes plus the blocking
 * flush() were removed. A short write means the half-open connection is gone, so
 * the client is dropped immediately instead of stalling the Web UI every cycle.
 */
static void httpServiceSse() {
  if (!sseStreamActive || !sseClient.connected()) {
    if (sseStreamActive) {
      sseStreamActive = false;
    }
    return;
  }

  const int8_t currentPlaybackState = (datamode == STOPPED) ? 0 : 1;
  const bool playbackStateChanged = (currentPlaybackState != sseLastPlaybackState);
  const uint32_t pushIntervalMs = 10000UL;

  if (!playbackStateChanged && (millis() - sseLastPushMs < pushIntervalMs)) {
    return;
  }

  sseLastPushMs = millis();
  sseLastPlaybackState = currentPlaybackState;

  // Send the whole event in a single write(): the WiFiClient write path uses a
  // non-blocking send() bounded by its own retry/select loop, so it cannot stall
  // the main loop indefinitely on a dead peer. A short write means the socket is
  // gone, so drop the client immediately instead of retrying every cycle. No
  // flush() is needed and no per-fragment writes that could each block.
  String frame = "data: " + buildLiveStatusJson() + "\n\n";
  size_t written = sseClient.print(frame);
  if (written < frame.length()) {
    sseClient.stop();
    sseStreamActive = false;
  }
}

#endif
