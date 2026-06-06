/**
 * @brief Global sleep timer value in milliseconds.
 *        -1 means timer is disabled.
 */
int32_t sleeptimer_ms = -1; // -1 means timer is disabled

/**
 * @brief Handles the sleep timer for automatically turning off the radio.
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
        dbgprint("SleepTimer running: %d", sleeptimer_ms / 1000);
    }
    
    if (sleeptimer_ms <= 0) {
      sleeptimer_ms = -1; // Reset timer
      if (datamode != STOPPED) {
        runStream(false);
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
    if (timeinfo.tm_hour == alrm_start_hour && 
        timeinfo.tm_min == alrm_start_min && 
        timeinfo.tm_sec == 0) {
        
        // 1. Use the configured alarm station (ini_block.newpreset)
        runStream(true, ini_block.newpreset);

        // 2. Signal that the MP3 loop should start immediately
        // (This also happens in the background on web click)
        hostreq = true;

        // 3. Turn on sound
        muteflag = false;

        // 4. Visual feedback
        enableBacklight(true);
        showTextOnTFT(1, msgRadioAlarmOn);
        radioAlarmStatusChanged = ALRM_START;
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
    if (timeinfo.tm_hour == alrm_stop_hour && 
        timeinfo.tm_min == alrm_stop_min && 
        timeinfo.tm_sec <= 2) {   // Only within the first 2 seconds of the minute
        
        runStream(false);
        showTextOnTFT(1, msgRadioAlarmOff);
        radioAlarmStatusChanged = ALRM_STOP;
        showTextOnTFT(3, msgPlayerStopped);
        enableBacklight(false);
        return true;
    }
    return false;
}

/**
 * @brief Updates the display one minute after an alarm start/stop event.
 * 
 * Shows the current Icy name when the alarm was started,
 * or clears the display when stopped.
 */
static void updateAlarmDisplayFeedback(void) {
    if (radioAlarmStatusChanged == ALRM_START || radioAlarmStatusChanged == ALRM_STOP) {
        radioAlarmStatusChanged = ALRM_NONE;
        
        if (radioAlarmStatusChanged == ALRM_START) {
            showTextOnTFT(3, icyname);  // Set screen segment bottom part
        } else {
            // showTextOnTFT(3, "");
        }
    }
}

/**
 * @brief Main function for handling the radio alarm.
 * 
 * Enables or disables the radio alarm based on the configured
 * start and stop times (ini_block.alrm_start / ini_block.alrm_stop).
 * 
 * This function checks:
 * - Whether an alarm is configured (string != "none", length >= 4)
 * - Whether the current system time matches start or stop time
 * - Then executes the corresponding action (playStream / stopStream)
 * 
 * @note Expects globally available 'timeinfo' (tm struct with hour/minute/second)
 * @note Uses ini_block.alrm_start and ini_block.alrm_stop as configuration sources
 * @note Updates radioAlarmStatusChanged for follow-up actions
 */
void handleRadioAlarm(void) {
    // Update display one minute after alarm start/stop
    updateAlarmDisplayFeedback();

    // --- RADIO ALARM START ---
    if (ini_block.alrm_start != "none" && ini_block.alrm_start.length() >= 4) {
        int8_t alrm_tm_start_hour = getValue(ini_block.alrm_start.c_str(), ':', 0).toInt();
        int8_t alrm_tm_start_min  = getValue(ini_block.alrm_start.c_str(), ':', 1).toInt();
        tryStartRadioAlarm(alrm_tm_start_hour, alrm_tm_start_min);
    }

    // --- RADIO ALARM STOP ---
    if (ini_block.alrm_stop.c_str() != "none" && ini_block.alrm_stop.length() >= 4) {
        int8_t alrm_tm_stop_hour = getValue(ini_block.alrm_stop.c_str(), ':', 0).toInt();
        int8_t alrm_tm_stop_min  = getValue(ini_block.alrm_stop.c_str(), ':', 1).toInt();
        tryStopRadioAlarm(alrm_tm_stop_hour, alrm_tm_stop_min);
    }
}