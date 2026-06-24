/**
 * @file captive_portal.h
 * @brief DNS hijack in AP mode so phones open the configuration page automatically.
 */

#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <DNSServer.h>
#include <WiFi.h>

static DNSServer captiveDns;
static bool captivePortalActive = false;

/**
 * @brief Stop captive DNS after STA connection succeeds.
 */
void captivePortalStop(void) {
  if (!captivePortalActive) {
    return;
  }
  captiveDns.stop();
  captivePortalActive = false;
}

/**
 * @brief Start captive DNS when running as access point.
 */
void captivePortalBegin(void) {
  if (WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
    captivePortalStop();
    return;
  }

  if (captivePortalActive) {
    return;
  }

  captiveDns.start(53, "*", WiFi.softAPIP());
  captivePortalActive = true;
  dbgprint("Captive portal DNS started on %s", WiFi.softAPIP().toString().c_str());
}

/**
 * @brief Process captive DNS requests (call from main loop).
 */
void captivePortalLoop(void) {
  if (captivePortalActive) {
    captiveDns.processNextRequest();
  }
}

/**
 * @brief Return true if the HTTP request should be redirected to the setup page.
 *
 * @param host Host header value
 * @param path Request path
 * @return bool
 */
bool captivePortalShouldRedirect(const String &host, const String &path) {
  if (!captivePortalActive) {
    return false;
  }

  if (path.indexOf("generate_204") >= 0 || path.indexOf("gen_204") >= 0 ||
      path.indexOf("hotspot-detect") >= 0 || path.indexOf("connecttest") >= 0 ||
      path.indexOf("ncsi") >= 0) {
    return true;
  }

  if (host.length() > 0 && host != "192.168.4.1" && host.indexOf("esp32") < 0) {
    return true;
  }

  return false;
}

#endif
