/**
 * @file title_history.h
 * @brief Song title history for Web UI JSON output.
 */

#ifndef TITLE_HISTORY_H
#define TITLE_HISTORY_H

/**
 * @brief Escape a string for safe inclusion in JSON values.
 *
 * @param input Null-terminated input string
 * @return String JSON-escaped string without surrounding quotes
 */
String jsonEscapeString(const char *input) {
  String out = "";

  if (input == nullptr) {
    return out;
  }

  for (const char *p = input; *p; p++) {
    char c = *p;

    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if ((unsigned char)c < 0x20) {
          char buf[7];
          sprintf(buf, "\\u%04x", (unsigned char)c);
          out += buf;
        } else {
          out += c;
        }
        break;
    }
  }

  return out;
}

/**
 * @brief Record a newly recognized stream title in the history list.
 *
 * Keeps the newest entry at index 0. When full, the oldest entry is dropped.
 * Consecutive identical titles are ignored.
 *
 * @param title Recognized stream title text (raw metadata, not display formatted)
 */
void addTitleToHistory(const char *title) {
  uint8_t slot;

  if (title == nullptr || title[0] == '\0') {
    return;
  }

  if (titleHistoryLastTitle.equals(title)) {
    return;
  }

  titleHistoryLastTitle = title;

  for (slot = TITLE_HISTORY_SLOTS - 1; slot > 0; slot--) {
    titleHistory[slot] = titleHistory[slot - 1];
  }

  strncpy(titleHistory[0].datetime, timetxt, sizeof(titleHistory[0].datetime) - 1);
  titleHistory[0].datetime[sizeof(titleHistory[0].datetime) - 1] = '\0';
  strncpy(titleHistory[0].title, title, sizeof(titleHistory[0].title) - 1);
  titleHistory[0].title[sizeof(titleHistory[0].title) - 1] = '\0';

  if (titleHistoryCount < TITLE_HISTORY_SLOTS) {
    titleHistoryCount++;
  }
}

/**
 * @brief Build the JSON fragment for the title history array.
 *
 * @return String JSON field \"history\":[...] without trailing comma
 */
String buildTitleHistoryJson() {
  String json = "\"history\":[";
  uint8_t i;

  for (i = 0; i < titleHistoryCount; i++) {
    if (i > 0) {
      json += ",";
    }

    json += "{\"datetime\":\"";
    json += jsonEscapeString(titleHistory[i].datetime);
    json += "\",\"title\":\"";
    json += jsonEscapeString(titleHistory[i].title);
    json += "\"}";
  }

  json += "]";
  return json;
}

#endif
