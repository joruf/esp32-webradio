/**
 * @file title_history.h
 * @brief Song title history and JSON string escaping for Web UI output.
 */

#ifndef TITLE_HISTORY_H
#define TITLE_HISTORY_H

namespace JsonUtil {

/**
 * @brief Escape a string for safe inclusion in JSON values.
 *
 * @param input Null-terminated input string
 * @return String JSON-escaped string without surrounding quotes
 */
inline String escape(const char *input) {
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

} // namespace JsonUtil

inline String jsonEscapeString(const char *input) {
  return JsonUtil::escape(input);
}

/**
 * @brief Ring buffer of recent stream titles for the Web UI.
 */
class TitleHistory {
public:
  static const uint8_t SLOT_COUNT = 10;

  struct Entry {
    char datetime[40];
    char title[150];
  };

  /**
   * @brief Record a newly recognized stream title.
   *
   * @param title Recognized stream title text
   */
  void add(const char *title) {
    uint8_t slot;

    if (title == nullptr || title[0] == '\0') {
      return;
    }

    if (lastTitle_.equals(title)) {
      return;
    }

    lastTitle_ = title;

    for (slot = SLOT_COUNT - 1; slot > 0; slot--) {
      entries_[slot] = entries_[slot - 1];
    }

    strncpy(entries_[0].datetime, timetxt, sizeof(entries_[0].datetime) - 1);
    entries_[0].datetime[sizeof(entries_[0].datetime) - 1] = '\0';
    strncpy(entries_[0].title, title, sizeof(entries_[0].title) - 1);
    entries_[0].title[sizeof(entries_[0].title) - 1] = '\0';

    if (count_ < SLOT_COUNT) {
      count_++;
    }
  }

  /**
   * @brief Build the JSON fragment for the title history array.
   *
   * @return String JSON field \"history\":[...] without trailing comma
   */
  String buildJsonFragment(void) const {
    String json = "\"history\":[";
    uint8_t i;

    for (i = 0; i < count_; i++) {
      if (i > 0) {
        json += ",";
      }

      json += "{\"datetime\":\"";
      json += JsonUtil::escape(entries_[i].datetime);
      json += "\",\"title\":\"";
      json += JsonUtil::escape(entries_[i].title);
      json += "\"}";
    }

    json += "]";
    return json;
  }

private:
  Entry entries_[SLOT_COUNT];
  uint8_t count_ = 0;
  String lastTitle_;
};

TitleHistory titleHistory;

inline void addTitleToHistory(const char *title) {
  titleHistory.add(title);
}

inline String buildTitleHistoryJson() {
  return titleHistory.buildJsonFragment();
}

#endif
