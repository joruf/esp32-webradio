#ifndef WEB_THEME_SERVE_H
#define WEB_THEME_SERVE_H

#include <Arduino.h>
#include <WiFi.h>

#include "web_assets/partials/nav_menu.h"

static const char HTML_DARK_HTML_ATTR[] PROGMEM = " style=\"background-color:black;\"";
static const char HTML_DARK_BODY_ATTR[] PROGMEM = " class=\"theme-dark\" style=\"background-color:black;\"";
static const char NAV_INCLUDE_PLACEHOLDER[] PROGMEM = "@include_nav@";

/**
 * @brief Find a PROGMEM substring inside a PROGMEM byte range.
 *
 * @param hay PROGMEM source buffer
 * @param hayLen Number of bytes in @p hay
 * @param needle PROGMEM substring to locate
 * @return int Start index of @p needle, or -1 if not found
 */
static int findProgmemSubstring(const char *hay, int hayLen, const char *needle) {
  const size_t needleLen = strlen_P(needle);

  if (needleLen == 0 || hayLen < (int)needleLen) {
    return -1;
  }

  for (int i = 0; i + (int)needleLen <= hayLen; i++) {
    if (strncmp_P(hay + i, needle, needleLen) == 0) {
      return i;
    }
  }

  return -1;
}

/**
 * @brief Find the closing '>' of an opening tag that starts with @p tag.
 *
 * @param progmemHtml HTML source in PROGMEM
 * @param length Number of bytes in @p progmemHtml
 * @param start Index to begin searching from
 * @param tag Opening tag prefix (e.g. "<html", "<body")
 * @return int Index of the closing '>', or -1 if not found
 */
static int findOpeningTagClose(const char *progmemHtml, int length, int start, const char *tag) {
  const size_t tagLen = strlen(tag);

  for (int i = start; i + (int)tagLen <= length; i++) {
    if (strncmp_P(progmemHtml + i, tag, tagLen) != 0) {
      continue;
    }

    for (int j = i + (int)tagLen; j < length; j++) {
      if (pgm_read_byte(progmemHtml + j) == '>') {
        return j;
      }
    }

    return -1;
  }

  return -1;
}

/**
 * @brief Extra bytes injected before <html> and <body> tag closers in dark mode.
 *
 * @param progmemHtml HTML source in PROGMEM
 * @param length Number of bytes to scan
 * @return int Additional bytes streamed when dark mode is active
 */
static int darkModeInjectedExtraBytes(const char *progmemHtml, int length) {
  int extra = 0;

  if (findOpeningTagClose(progmemHtml, length, 0, "<html") >= 0) {
    extra += (int)strlen_P(HTML_DARK_HTML_ATTR);
  }
  if (findOpeningTagClose(progmemHtml, length, 0, "<body") >= 0) {
    extra += (int)strlen_P(HTML_DARK_BODY_ATTR);
  }

  return extra;
}

/**
 * @brief Compute the exact number of bytes streamed for an HTML page.
 *
 * Accounts for @include_nav@ expansion and optional dark-mode attribute injection.
 *
 * @param progmemPage Page source in PROGMEM
 * @param rawLength Stored PROGMEM length (typically sizeof(symbol) - 1)
 * @param darkMode True when dark-mode attributes are injected into <html>/<body>
 * @return int Total response body length in bytes
 */
static int servedHtmlLength(const char *progmemPage, int rawLength, bool darkMode) {
  const int navPos = findProgmemSubstring(progmemPage, rawLength, NAV_INCLUDE_PLACEHOLDER);

  if (navPos < 0) {
    int len = rawLength;
    if (darkMode) {
      len += darkModeInjectedExtraBytes(progmemPage, rawLength);
    }
    return len;
  }

  const int placeholderLen = (int)strlen_P(NAV_INCLUDE_PLACEHOLDER);
  const int navLen = (int)(sizeof(web_nav_menu_html) - 1);
  const int tailLen = rawLength - navPos - placeholderLen;
  int len = navPos + navLen + tailLen;

  if (darkMode) {
    len += darkModeInjectedExtraBytes(progmemPage, navPos);
  }

  return len;
}

/**
 * @brief Compute the served HTML length when @include_nav@ is expanded at runtime.
 *
 * @param progmemPage Page source in PROGMEM
 * @param rawLength Stored PROGMEM length (typically sizeof(symbol) - 1)
 * @return int Length of the page as sent to the browser without dark-mode injection
 */
static int expandedHtmlLength(const char *progmemPage, int rawLength) {
  return servedHtmlLength(progmemPage, rawLength, false);
}

/**
 * @brief Write a byte buffer to a client, retrying partial TCP writes.
 *
 * @param client Active HTTP client
 * @param data Source bytes (RAM)
 * @param length Number of bytes to send
 * @return bool False if the client disconnected before all bytes were sent
 */
static bool writeClientBytes(WiFiClient &client, const uint8_t *data, int length) {
  int sent = 0;
  uint16_t idleRetries = 0;

  while (sent < length) {
    if (!client.connected()) {
      return false;
    }

    const int n = client.write(data + sent, (size_t)(length - sent));
    if (n > 0) {
      sent += n;
      idleRetries = 0;
      yield();
      continue;
    }

    if (++idleRetries > 200) {
      return false;
    }
    delay(1);
  }

  return true;
}

/**
 * @brief Write a PROGMEM C string to a connected client.
 *
 * @param client Active HTTP client
 * @param progmemStr NUL-terminated string in PROGMEM
 */
static void writeProgmemCString(WiFiClient &client, const char *progmemStr) {
  char buffer[64];
  size_t bufLen = 0;
  size_t pos = 0;

  for (;;) {
    const char c = pgm_read_byte(progmemStr + pos++);
    if (c == '\0') {
      break;
    }

    buffer[bufLen++] = c;
    if (bufLen >= sizeof(buffer)) {
      if (!writeClientBytes(client, (const uint8_t *)buffer, (int)bufLen)) {
        return;
      }
      bufLen = 0;
      yield();
    }
  }

  if (bufLen > 0) {
    writeClientBytes(client, (const uint8_t *)buffer, (int)bufLen);
  }
}

/**
 * @brief Write a byte range from PROGMEM in chunks.
 *
 * Partial TCP writes are retried so a full chunk is never skipped when the
 * socket buffer is momentarily full (e.g. while several assets load in parallel).
 *
 * @param client Active HTTP client
 * @param progmemData Source data in PROGMEM
 * @param length Number of bytes to write
 * @param chunkSize Maximum bytes per write attempt
 * @return bool False if the client disconnected before the range was sent
 */
static bool writeProgmemRange(WiFiClient &client, const char *progmemData, int length, int chunkSize) {
  char buffer[1024];
  const int maxChunk = (int)sizeof(buffer);
  int flushedSince = 0;

  if (chunkSize > maxChunk) {
    chunkSize = maxChunk;
  }

  while (length > 0) {
    if (!client.connected()) {
      return false;
    }

    const int chunk = (length <= chunkSize) ? length : chunkSize;
    for (int i = 0; i < chunk; i++) {
      buffer[i] = pgm_read_byte(progmemData + i);
    }

    if (!writeClientBytes(client, (const uint8_t *)buffer, chunk)) {
      return false;
    }

    progmemData += chunk;
    length -= chunk;
    flushedSince += chunk;

    // Flush periodically; a flush on every 1 KiB blocked the main loop during
    // parallel asset downloads and made large responses stall near the end.
    if (flushedSince >= 8192) {
      client.flush();
      flushedSince = 0;
      yield();
    }
  }

  if (flushedSince > 0) {
    client.flush();
  }

  return true;
}

/**
 * @brief Stream HTML from PROGMEM with inline dark-mode styles on <html> and <body>.
 *
 * @param client Active HTTP client
 * @param progmemHtml HTML source in PROGMEM
 * @param length Number of bytes in @p progmemHtml
 * @param chunkSize Maximum bytes per write for unmodified ranges
 * @return bool False if the client disconnected before the page was sent
 */
static bool streamHtmlWithDarkMode(WiFiClient &client, const char *progmemHtml, int length, int chunkSize) {
  const int htmlClose = findOpeningTagClose(progmemHtml, length, 0, "<html");
  const int bodyClose = findOpeningTagClose(progmemHtml, length, 0, "<body");
  int pos = 0;

  if (htmlClose >= 0) {
    if (!writeProgmemRange(client, progmemHtml + pos, htmlClose - pos, chunkSize)) {
      return false;
    }

    writeProgmemCString(client, HTML_DARK_HTML_ATTR);
    client.write('>');
    client.flush();
    yield();
    pos = htmlClose + 1;
  }

  if (bodyClose >= pos) {
    if (!writeProgmemRange(client, progmemHtml + pos, bodyClose - pos, chunkSize)) {
      return false;
    }

    writeProgmemCString(client, HTML_DARK_BODY_ATTR);
    client.write('>');
    client.flush();
    yield();
    pos = bodyClose + 1;
  }

  return writeProgmemRange(client, progmemHtml + pos, length - pos, chunkSize);
}

/**
 * @brief Stream HTML from PROGMEM, expanding @include_nav@ to web_nav_menu_html.
 *
 * @param client Active HTTP client
 * @param progmemHtml HTML source in PROGMEM
 * @param rawLength Number of bytes in @p progmemHtml
 * @param darkMode True when the configured dark-mode window is active
 * @param chunkSize Maximum bytes per write for unmodified ranges
 * @return bool False if the client disconnected before the page was sent
 */
static bool streamHtmlWithNavInclude(WiFiClient &client, const char *progmemHtml, int rawLength, bool darkMode,
                                     int chunkSize) {
  const int navPos = findProgmemSubstring(progmemHtml, rawLength, NAV_INCLUDE_PLACEHOLDER);

  if (navPos < 0) {
    if (darkMode) {
      return streamHtmlWithDarkMode(client, progmemHtml, rawLength, chunkSize);
    }

    return writeProgmemRange(client, progmemHtml, rawLength, chunkSize);
  }

  const int placeholderLen = (int)strlen_P(NAV_INCLUDE_PLACEHOLDER);
  const int navLen = (int)(sizeof(web_nav_menu_html) - 1);
  const int tailLen = rawLength - navPos - placeholderLen;

  if (darkMode) {
    if (!streamHtmlWithDarkMode(client, progmemHtml, navPos, chunkSize)) {
      return false;
    }
  } else if (!writeProgmemRange(client, progmemHtml, navPos, chunkSize)) {
    return false;
  }

  if (!writeProgmemRange(client, web_nav_menu_html, navLen, chunkSize)) {
    return false;
  }

  return writeProgmemRange(client, progmemHtml + navPos + placeholderLen, tailLen, chunkSize);
}

/**
 * @brief Stream a web page from PROGMEM, injecting dark-mode markup for HTML when needed.
 *
 * @param client Active HTTP client
 * @param progmemPage Page source in PROGMEM
 * @param length Number of bytes in @p progmemPage
 * @param contentType MIME type of the response (e.g. "text/html")
 * @param darkMode True when the configured dark-mode window is active
 * @param chunkSize Maximum bytes per write
 * @return bool False if the client disconnected before the page was sent
 */
static bool streamWebPage(WiFiClient &client, const char *progmemPage, int length, const String &contentType,
                          bool darkMode, int chunkSize) {
  if (contentType == "text/html" && length >= 10) {
    return streamHtmlWithNavInclude(client, progmemPage, length, darkMode, chunkSize);
  }

  return writeProgmemRange(client, progmemPage, length, chunkSize);
}

#endif
