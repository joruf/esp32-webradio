#ifndef WEB_THEME_SERVE_H
#define WEB_THEME_SERVE_H

#include <Arduino.h>
#include <WiFi.h>

static const char HTML_DARK_HTML_ATTR[] PROGMEM = " style=\"background-color:black;\"";
static const char HTML_DARK_BODY_ATTR[] PROGMEM = " class=\"theme-dark\" style=\"background-color:black;\"";

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
 * @brief Write a PROGMEM C string to a connected client.
 *
 * @param client Active HTTP client
 * @param progmemStr NUL-terminated string in PROGMEM
 */
static void writeProgmemCString(WiFiClient &client, const char *progmemStr) {
  char buffer[64];
  size_t offset = 0;

  for (;;) {
    const char c = pgm_read_byte(progmemStr + offset);
    if (c == '\0') {
      break;
    }

    buffer[offset++] = c;
    if (offset >= sizeof(buffer)) {
      client.write((const uint8_t *)buffer, offset);
      offset = 0;
      yield();
    }
  }

  if (offset > 0) {
    client.write((const uint8_t *)buffer, offset);
  }
}

/**
 * @brief Write a byte range from PROGMEM in chunks.
 *
 * @param client Active HTTP client
 * @param progmemData Source data in PROGMEM
 * @param length Number of bytes to write
 * @param chunkSize Maximum bytes per write
 * @return bool False if the client disconnected before the range was sent
 */
static bool writeProgmemRange(WiFiClient &client, const char *progmemData, int length, int chunkSize) {
  const char *p = progmemData;

  while (length > 0) {
    if (!client.connected()) {
      return false;
    }

    const int chunk = (length <= chunkSize) ? length : chunkSize;
    client.write((const uint8_t *)p, chunk);
    client.flush();
    yield();

    p += chunk;
    length -= chunk;
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
  if (darkMode && contentType == "text/html" && length >= 10) {
    return streamHtmlWithDarkMode(client, progmemPage, length, chunkSize);
  }

  return writeProgmemRange(client, progmemPage, length, chunkSize);
}

#endif
