/**
 * @file captive_portal.h
 * @brief DNS hijack in AP mode so phones open the configuration page automatically.
 */

#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <DNSServer.h>
#include <WiFi.h>

/**
 * @brief Captive DNS portal for WiFi access-point provisioning.
 */
class CaptivePortal {
public:
  /**
   * @brief Stop captive DNS after STA connection succeeds.
   */
  void stop(void) {
    if (!active_) {
      return;
    }
    dns_.stop();
    active_ = false;
  }

  /**
   * @brief Start captive DNS when running as access point.
   */
  void begin(void) {
    if (WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
      stop();
      return;
    }

    if (active_) {
      return;
    }

    dns_.start(53, "*", WiFi.softAPIP());
    active_ = true;
    dbgprint("Captive portal DNS started on %s", WiFi.softAPIP().toString().c_str());
  }

  /**
   * @brief Process captive DNS requests (call from main loop).
   */
  void loop(void) {
    if (active_) {
      dns_.processNextRequest();
    }
  }

  /**
   * @brief Return true if the HTTP request should be redirected to the setup page.
   *
   * @param host Host header value
   * @param path Request path
   * @return bool
   */
  bool shouldRedirect(const String &host, const String &path) const {
    if (!active_) {
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

  bool isActive(void) const {
    return active_;
  }

private:
  DNSServer dns_;
  bool active_ = false;
};

CaptivePortal captivePortal;

inline void captivePortalStop(void) {
  captivePortal.stop();
}

inline void captivePortalBegin(void) {
  captivePortal.begin();
}

inline void captivePortalLoop(void) {
  captivePortal.loop();
}

inline bool captivePortalShouldRedirect(const String &host, const String &path) {
  return captivePortal.shouldRedirect(host, path);
}

#endif
