//**************************************************************************************************
//                                         G E T T I M E *
//**************************************************************************************************
// Retrieve the local time from NTP server and convert to string. * Will be
// called every second. *
//**************************************************************************************************

/**
 * @brief Format timeinfo into timetxt string.
 *
 * @param days Array of day name abbreviations
 * @return true if time is valid, false otherwise
 */
static bool formatTimeString(const char *days[]) {
  if (!timeinfo.tm_year) {
    return false;
  }

  sprintf(timetxt, "%s %02d.%02d.%02d %02d:%02d", days[timeinfo.tm_wday],
          timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year % 100,
          timeinfo.tm_hour, timeinfo.tm_min);

  return true;
}

/**
 * @brief Synchronize time with NTP server.
 *
 * @param retrycount Reference to retry counter
 * @param delaycount Reference to delay counter
 * @param days Array of day name abbreviations
 */
static void syncTimeWithNtp(int16_t &retrycount, int16_t &delaycount,
                            const char *days[]) {
  dbgprint("Sync TOD");

  if (getLocalTime(&timeinfo)) {
    formatTimeString(days);
  } else {
    dbgprint("Failed to obtain time!");
    timeinfo.tm_year = 0;

    if (retrycount) {
      retrycount--;
      delaycount = 5;
    }
  }
}

/**
 * @brief Retrieve the local time from NTP server and convert to string.
 *        Will be called every second.
 */
void getCurrentTime() {
  static int16_t delaycount = 0; // To reduce number of NTP requests
  static int16_t retrycount = 100;
  static const char *days[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

  if (!tft) {
    return;
  }

  // Format current time if valid
  if (timeinfo.tm_year) {
    formatTimeString(days);
  }

  // Sync every few hours
  if (--delaycount <= 0) {
    delaycount = 7200; // Reset counter (7200 * 1 second = 2 hours)

    if (timeinfo.tm_year) {
      dbgprint("Sync TOD, old value is %s", timetxt);
    }

    syncTimeWithNtp(retrycount, delaycount, days);
  }
}