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
  client.flush();
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
  httpBeginSse(sseClient);
  sseStreamActive = true;
  sseLastPushMs = 0;
}

/**
 * @brief Push a live JSON event to the active SSE client.
 */
static void httpServiceSse() {
  if (!sseStreamActive || !sseClient.connected()) {
    if (sseStreamActive) {
      sseStreamActive = false;
    }
    return;
  }

  if (millis() - sseLastPushMs < 2000) {
    return;
  }

  sseLastPushMs = millis();
  String payload = buildLiveStatusJson();
  sseClient.print("data: ");
  sseClient.print(payload);
  sseClient.print("\n\n");
  sseClient.flush();
}

#endif
