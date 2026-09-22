//**************************************************************************************************
//                                         G E T T I M E *
//**************************************************************************************************
// Retrieve the local time from NTP server and convert to string. * Will be
// called every second. *
//**************************************************************************************************

#include <esp_sntp.h>

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

  sprintf(timetxt, "%s %02d.%02d.%02d %02d:%02d", days[timeinfo.tm_wday], timeinfo.tm_mday, timeinfo.tm_mon + 1,
          timeinfo.tm_year % 100, timeinfo.tm_hour, timeinfo.tm_min);

  return true;
}

/**
 * @brief Retrieve the local time from the system clock (SNTP) and convert to string.
 *        Called once per second when time_req is set.
 */
void getCurrentTime() {
  static int16_t logcount = 0;
  static const char *days[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

  if (!NetworkFound) {
    return;
  }

  if (getLocalTime(&timeinfo)) {
    formatTimeString(days);
  } else {
    timeinfo.tm_year = 0;
  }

  if (--logcount <= 0) {
    logcount = 300;
    if (timeinfo.tm_year) {
      dbgprint("Clock: %s (wday %d)", timetxt, timeinfo.tm_wday);
    } else {
      dbgprint("Clock: waiting for NTP sync");
    }
  }
}

/**
 * @brief Apply clock settings from ini_block and refresh the system time.
 *
 * Reconfigures timezone/NTP from ini_block. Offset and DST changes take effect
 * immediately via getLocalTime(). When restartNtp is true, SNTP is restarted
 * (e.g. after changing the NTP server).
 *
 * @param restartNtp Restart SNTP and force an immediate resync
 */
void applyClockConfiguration(bool restartNtp) {
  static const char *days[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

  configTime(ini_block.clk_offset * 3600, ini_block.clk_dst * 3600, ini_block.clk_server.c_str());

  if (!NetworkFound) {
    return;
  }

  if (restartNtp && esp_sntp_enabled()) {
    sntp_restart();
  }

  if (getLocalTime(&timeinfo, 200) && formatTimeString(days)) {
    time_req = true;
  }
}
