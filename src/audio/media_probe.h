/**
 * @file media_probe.h
 * @brief Proxy-scan a LAN HTTP directory listing for MP3 files.
 */

#ifndef MEDIA_PROBE_H
#define MEDIA_PROBE_H

#include "title_history.h"
#include <WiFi.h>
#include <WiFiClient.h>

static const uint16_t MEDIA_PROBE_MAX_FILES = 64;
static const uint32_t MEDIA_PROBE_READ_TIMEOUT_MS = 8000UL;

/**
 * @brief Extract a query parameter value from an HTTP command string.
 *
 * @param httpGetcmd Full query command (e.g. "mediadir=http://...")
 * @param paramName Parameter name without "="
 * @return String Decoded parameter value, or empty string if missing
 */
static String mediaProbeQueryParam(const String &httpGetcmd, const char *paramName) {
  String key = String(paramName) + String("=");
  int keyPos = httpGetcmd.indexOf(key);
  if (keyPos < 0) {
    return "";
  }

  String value = httpGetcmd.substring(keyPos + key.length());
  int amp = value.indexOf('&');
  if (amp >= 0) {
    value = value.substring(0, amp);
  }
  value.trim();
  return value;
}

/**
 * @brief Decode percent-encoded sequences (%XX) in a value to plain bytes.
 *
 * @param text Percent-encoded input string
 * @return String Decoded result string
 */
static String mediaProbeUrlDecode(const String &text) {
  String decoded;
  size_t i = 0;

  decoded.reserve(text.length());
  while (i < text.length()) {
    char c = text[i];
    if ((c == '%') && (i + 2 < text.length())) {
      char h1 = text[i + 1];
      char h2 = text[i + 2];
      if (isxdigit((unsigned char)h1) && isxdigit((unsigned char)h2)) {
        char tmp[3];
        tmp[0] = h1;
        tmp[1] = h2;
        tmp[2] = '\0';
        decoded += (char)strtol(tmp, nullptr, 16);
        i += 3;
        continue;
      }
    }
    decoded += c;
    i++;
  }

  return decoded;
}

/**
 * @brief Parse a base HTTP URL into host, port, and path components.
 *
 * @param baseUrl Base URL such as http://192.168.1.20:8000/music/
 * @param hostOut Output host name or IP
 * @param portOut Output TCP port
 * @param pathOut Output path starting with "/"
 * @return true when parsing succeeded
 */
static bool mediaProbeParseBaseUrl(const String &baseUrl, String &hostOut, uint16_t &portOut, String &pathOut) {
  String url = baseUrl;
  url.trim();
  url.toLowerCase();

  if (url.startsWith("https://")) {
    return false;
  }
  if (url.startsWith("http://")) {
    url = baseUrl.substring(7);
    url.trim();
  }

  int slash = url.indexOf('/');
  String hostPort = (slash >= 0) ? url.substring(0, slash) : url;
  pathOut = (slash >= 0) ? url.substring(slash) : String("/");
  if (!pathOut.startsWith("/")) {
    pathOut = String("/") + pathOut;
  }
  if (!pathOut.endsWith("/")) {
    pathOut += "/";
  }

  int colon = hostPort.indexOf(':');
  if (colon >= 0) {
    hostOut = hostPort.substring(0, colon);
    portOut = (uint16_t)hostPort.substring(colon + 1).toInt();
  } else {
    hostOut = hostPort;
    portOut = 80;
  }

  hostOut.trim();
  return hostOut.length() > 0 && portOut > 0;
}

/**
 * @brief Check whether a href should be listed as an MP3 track.
 *
 * @param href Relative or absolute href from a directory listing
 * @return true when the href points to an .mp3 file
 */
static bool mediaProbeIsMp3Href(const String &href) {
  String normalized = href;
  int queryPos = normalized.indexOf('?');
  if (queryPos >= 0) {
    normalized = normalized.substring(0, queryPos);
  }
  normalized.toLowerCase();
  return normalized.endsWith(".mp3");
}

/**
 * @brief Append a unique MP3 href to the output list.
 *
 * @param files Output href list
 * @param fileCount Current number of stored hrefs
 * @param href Candidate href
 * @return uint16_t Updated file count
 */
static uint16_t mediaProbeAppendUniqueHref(String *files, uint16_t fileCount, const String &href) {
  String candidate = href;
  candidate.trim();
  if (!mediaProbeIsMp3Href(candidate) || candidate == "/" || candidate == "../") {
    return fileCount;
  }

  for (uint16_t i = 0; i < fileCount; i++) {
    if (files[i].equalsIgnoreCase(candidate)) {
      return fileCount;
    }
  }

  if (fileCount >= MEDIA_PROBE_MAX_FILES) {
    return fileCount;
  }

  files[fileCount] = candidate;
  return fileCount + 1;
}

/**
 * @brief Collect MP3 href values from a simple HTML directory listing.
 *
 * @param html HTML body text
 * @param files Output href list
 * @return uint16_t Number of MP3 files found
 */
static uint16_t mediaProbeCollectMp3Hrefs(const String &html, String *files) {
  uint16_t fileCount = 0;
  int searchPos = 0;

  while (searchPos < (int)html.length()) {
    int hrefPos = html.indexOf("href=\"", searchPos);
    if (hrefPos < 0) {
      break;
    }

    int valueStart = hrefPos + 6;
    int valueEnd = html.indexOf('"', valueStart);
    if (valueEnd < 0) {
      break;
    }

    fileCount = mediaProbeAppendUniqueHref(files, fileCount, html.substring(valueStart, valueEnd));
    searchPos = valueEnd + 1;
  }

  return fileCount;
}

/**
 * @brief Build JSON for a LAN media-directory probe request.
 *
 * @param baseUrl Base URL of the media server folder
 * @return String JSON response document
 */
static String buildMediaDirectoryProbeJson(const String &baseUrl) {
  String host;
  String path;
  uint16_t port = 80;
  WiFiClient client;
  String body;
  String files[MEDIA_PROBE_MAX_FILES];
  uint16_t fileCount = 0;
  uint32_t deadline;
  char buffer[256];

  if (!mediaProbeParseBaseUrl(baseUrl, host, port, path)) {
    return String("{\"ok\":false,\"error\":\"") + jsonEscapeString("Invalid Base URL. Use http://IP:PORT/") + "\"}";
  }

  if (!client.connect(host.c_str(), port)) {
    return String("{\"ok\":false,\"error\":\"") +
           jsonEscapeString("Could not connect to media server") + "\"}";
  }

  client.print(String("GET ") + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n");
  deadline = millis() + MEDIA_PROBE_READ_TIMEOUT_MS;

  while (client.connected() || client.available()) {
    if ((int)body.length() > 32768) {
      break;
    }
    if (millis() > deadline) {
      break;
    }

    int available = client.available();
    if (available <= 0) {
      yield();
      continue;
    }

    int toRead = available;
    if (toRead > (int)sizeof(buffer) - 1) {
      toRead = sizeof(buffer) - 1;
    }
    int got = client.readBytes(buffer, toRead);
    if (got <= 0) {
      continue;
    }
    buffer[got] = '\0';
    body += String(buffer);
  }

  client.stop();

  int bodyStart = body.indexOf("\r\n\r\n");
  if (bodyStart >= 0) {
    body = body.substring(bodyStart + 4);
  }

  fileCount = mediaProbeCollectMp3Hrefs(body, files);

  if (fileCount == 0) {
    return String("{\"ok\":false,\"message\":\"") +
           jsonEscapeString("No MP3 files found in this folder.") + "\"}";
  }

  String json = "{\"ok\":true,\"files\":[";
  for (uint16_t i = 0; i < fileCount; i++) {
    if (i > 0) {
      json += ",";
    }
    json += "\"" + jsonEscapeString(files[i].c_str()) + "\"";
  }
  json += "]}";
  return json;
}

#endif
