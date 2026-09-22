/**
 * @brief Global sleep timer value in milliseconds.
 *        -1 means timer is disabled.
 */
int32_t sleeptimer_ms = -1; // -1 means timer is disabled

/** Unix epoch (seconds) when the snooze timer stops playback; 0 if inactive. */
time_t snooze_off_epoch = 0;

/** Original snooze duration in minutes (dropdown value); 0 if inactive. */
int16_t snooze_duration_mins = 0;

static int8_t alarm_last_check_hour = -1;
static int8_t alarm_last_check_min = -1;
static uint16_t alarm_started_mask = 0;
static uint16_t alarm_stopped_mask = 0;

void clearSnoozeTimer(void);

/**
 * @brief Format the snooze shut-off time as HH:MM for the Web UI.
 *
 * @return Time label or empty string when no snooze is active or clock is unset
 */
String formatSnoozeOffAt(void) {
  if (snooze_off_epoch <= 0) {
    return "";
  }

  struct tm ti;
  if (!localtime_r(&snooze_off_epoch, &ti)) {
    return "";
  }

  char buf[8];
  snprintf(buf, sizeof(buf), "%02d:%02d", ti.tm_hour, ti.tm_min);
  return String(buf);
}

/**
 * @brief Start or clear the snooze (sleep) timer.
 *
 * @param mins Selected duration in minutes; 0 disables the timer
 */
void setSnoozeTimerMinutes(int mins) {
  if (mins > 0) {
    sleeptimer_ms = (int32_t)mins * 60 * 1000;
    snooze_duration_mins = (int16_t)mins;
    time_t now = time(nullptr);
    if (now > 1000000000L) {
      snooze_off_epoch = now + (time_t)(mins * 60);
    } else {
      snooze_off_epoch = 0;
    }
    dbgprint("Snooze set to %d min (off at epoch %lu)", mins, (unsigned long)snooze_off_epoch);
    return;
  }

  clearSnoozeTimer();
}

/**
 * @brief Disable the snooze timer and clear stored shut-off metadata.
 */
void clearSnoozeTimer(void) {
  sleeptimer_ms = -1;
  snooze_off_epoch = 0;
  snooze_duration_mins = 0;
  dbgprint("Snooze off");
}

/**
 * @brief Reset alarm trigger tracking after schedule changes.
 */
void resetAlarmScheduleTracking(void) {
  alarm_last_check_hour = -1;
  alarm_last_check_min = -1;
  alarm_started_mask = 0;
  alarm_stopped_mask = 0;
}

/**
 * @brief Reset per-minute alarm trigger tracking when the clock minute changes.
 */
static void resetAlarmMinuteTracking(void) {
  if (timeinfo.tm_hour != alarm_last_check_hour || timeinfo.tm_min != alarm_last_check_min) {
    alarm_last_check_hour = timeinfo.tm_hour;
    alarm_last_check_min = timeinfo.tm_min;
    alarm_started_mask = 0;
    alarm_stopped_mask = 0;
  }
}

/**
 * @brief Handles the snooze timer (formerly sleep timer) for automatically turning off the radio.
 *
 * This function must be called regularly (e.g., in the main loop).
 * It reduces the remaining sleep timer by the elapsed time and
 * stops playback when the timer expires.
 *
 * @note Uses millis() for time dependencies.
 * @note When timer is active (sleeptimer_ms > 0), elapsed time is subtracted.
 *       When timer expires, stopStream() is called.
 * @note When timer is inactive (sleeptimer_ms <= 0), only the internal
 *       time base is updated.
 */
void handleSleepTimer() {
  static uint32_t last_millis = 0;
  static uint32_t last_debug_output = 0;

  if (sleeptimer_ms > 0) {
    uint32_t current_millis = millis();
    uint32_t diff = current_millis - last_millis;
    last_millis = current_millis;

    sleeptimer_ms -= diff;

    // Debug output only once per second
    if (current_millis - last_debug_output >= 1000) {
      last_debug_output = current_millis;
      dbgprint("Snooze timer running: %d", sleeptimer_ms / 1000);
    }

    if (sleeptimer_ms <= 0) {
      clearSnoozeTimer();
      if (datamode != STOPPED) {
        // Gentle "sleep" shutdown: fade the volume down over the configured
        // "Alarm volume fade" duration, then stop (mirrors the alarm fade-in).
        dbgprint("Snooze timer expired, fading out over %d s", radioFeatures.alarm_fade_sec);
#if !defined(BLUETFT)
        showTextOnTFT(1, "Stream stopped");
#endif
        showTextOnTFT(3, "Radio off");
        startAlarmVolumeFadeOut(radioFeatures.alarm_fade_sec);
      }
    }
  } else {
    last_millis = millis(); // Timer idle, keep time base current
  }
}

/**
 * @brief Attempts to start the radio alarm when the current time matches
 *        the configured start time.
 *
 * @param alrm_start_hour   Configured start hour (0-23)
 * @param alrm_start_min    Configured start minute (0-59)
 * @return true  if alarm was started, otherwise false
 */
static bool tryStartRadioAlarm(int8_t alrm_start_hour, int8_t alrm_start_min) {
  if (timeinfo.tm_hour == alrm_start_hour && timeinfo.tm_min == alrm_start_min) {

    setAlarmSessionActive(true);
    streamWatchdogReset();
    stopAlarmFallback();

    runStream(true, ini_block.newpreset);

    hostreq = true;

    muteflag = false;
    startAlarmVolumeFade(radioFeatures.alarm_fade_sec);

    enableBacklight(true);
    showTextOnTFT(1, msgRadioAlarmOn);
    radioAlarmStatusChanged = ALRM_START;
    dbgprint("Radio alarm started at %02d:%02d", alrm_start_hour, alrm_start_min);
    return true;
  }
  return false;
}

/**
 * @brief Attempts to stop the radio alarm when the current time matches
 *        the configured stop time.
 *
 * The check occurs within the first 2 seconds of the target minute
 * to ensure reliable stopping.
 *
 * @param alrm_stop_hour    Configured stop hour (0-23)
 * @param alrm_stop_min     Configured stop minute (0-59)
 * @return true  if alarm was stopped, otherwise false
 */
static bool tryStopRadioAlarm(int8_t alrm_stop_hour, int8_t alrm_stop_min) {
  if (timeinfo.tm_hour == alrm_stop_hour && timeinfo.tm_min == alrm_stop_min) {
    endAlarmSession();
    runStream(false);
    showTextOnTFT(1, msgRadioAlarmOff);
    radioAlarmStatusChanged = ALRM_STOP;
    showTextOnTFT(3, msgPlayerStopped);
    dbgprint("Radio alarm stopped at %02d:%02d", alrm_stop_hour, alrm_stop_min);
    return true;
  }
  return false;
}

/**
 * @brief Updates the display after an alarm start/stop event.
 */
static void updateAlarmDisplayFeedback(void) {
  if (radioAlarmStatusChanged == ALRM_START) {
    radioAlarmStatusChanged = ALRM_NONE;
    String stationName = metaGetStation();
    showTextOnTFT(3, stationName);
  } else if (radioAlarmStatusChanged == ALRM_STOP) {
    radioAlarmStatusChanged = ALRM_NONE;
  }
}

/**
 * @brief Main function for handling the radio alarm.
 *
 * Evaluates all enabled alarm schedules from alarm_schedule and starts or
 * stops playback when the current weekday and time match a configured entry.
 *
 * @note Expects globally available timeinfo (tm struct with hour/minute/second)
 * @note Updates radioAlarmStatusChanged for follow-up actions
 */
void handleRadioAlarm(void) {
  if (!timeinfo.tm_year) {
    return;
  }

  updateAlarmDisplayFeedback();
  resetAlarmMinuteTracking();

  const int8_t weekday = timeinfo.tm_wday;

  for (uint8_t i = 0; i < alarm_schedule.count; i++) {
    const AlarmEntry &entry = alarm_schedule.entries[i];
    const uint16_t alarm_bit = (uint16_t)(1U << i);

    if (!entry.enabled) {
      continue;
    }
    if (entry.days_mask == 0) {
      continue;
    }
    if (!(entry.days_mask & (1 << weekday))) {
      continue;
    }

    if (!(alarm_started_mask & alarm_bit)) {
      if (tryStartRadioAlarm(entry.start_hour, entry.start_min)) {
        alarm_started_mask |= alarm_bit;
      }
    }

    if (!(alarm_stopped_mask & alarm_bit)) {
      if (tryStopRadioAlarm(entry.stop_hour, entry.stop_min)) {
        alarm_stopped_mask |= alarm_bit;
      }
    }
  }
}
