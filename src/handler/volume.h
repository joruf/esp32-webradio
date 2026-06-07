/**
 * @brief Checks if sound is currently enabled (not muted).
 *
 * @return true  If sound is enabled (muteflag is false)
 * @return false If sound is muted (muteflag is true)
 */
bool isSoundPlaying() {
  if (muteflag == false) {
    return true;
  }

  return false;
}

/**
 * @brief Toggles the sound state (play or mute).
 *
 * Updates the muteflag and refreshes the TFT display accordingly.
 *
 * @param _enable true to enable sound (unmute), false to mute the sound
 */
void playSound(bool _enable) {
  if (_enable == true) {
    muteflag = false;          // Mute/unmute: boolean{mute=true, play=false}
    showTextOnTFT(3, icyname); // Set screen segment bottom part
  } else {
    muteflag = true;          // Mute/unmute: boolean{mute=true, play=false}
    showTextOnTFT(3, "Mute"); // Set screen segment bottom part
  }
}

/**
 * @brief Extracts a field from a delimited string.
 *
 * Searches for the 'index'-th occurrence of 'separator' in 'data' and
 * returns the substring between that delimiter and the next one.
 *
 * @param data      String to search through
 * @param separator Delimiter character (e.g. ':', ',', ';')
 * @param index     Desired field position (0 = first field)
 * @return String   The extracted field, or empty string if index does not exist
 */
uint8_t getVolume() {
  return ini_block.reqvol;
  // vs1053player->getVolume();
}

/**
 * @brief Sets the absolute volume level.
 *
 * Updates the global volume setting if the provided value is within valid bounds.
 *
 * @param volume The new volume level (0-100)
 * @return true  If the volume was successfully set
 * @return false If the volume value was out of range
 */
bool setVolume(uint8_t volume) {
  if (volume < 0 || volume > 100) {
    return false;
  }

  ini_block.reqvol = volume;
  return true;
}

/**
 * @brief Adjusts the volume by a specified relative amount.
 *
 * Adds the given delta to the current volume. The result is constrained
 * to the range [0, 100].
 *
 * @param delta The value to add (positive to increase, negative to decrease)
 * @return uint8_t The new volume level
 */
uint8_t adjustVolume(int8_t delta) {
  int16_t newVolume = (int16_t)ini_block.reqvol + delta;

  ini_block.reqvol = (uint8_t)constrain(newVolume, 0, 100);

  return ini_block.reqvol;
}

/**
 * @desc	Check if stream is running
 * */
bool isStreamPlaying() { return datamode != STOPPED; }

/**
 * @brief Controls audio playback (start/stop).
 *
 * This function starts or stops the audio stream based on the 'play' parameter.
 * It updates the display accordingly and manages the backlight.
 *
 * @details
 * - If play = true: Starts playback using the specified preset or current
 * preset
 * - If play = false: Stops the current playback
 * - Updates the TFT display to show current status (Stream started/stopped,
 * Radio on/off)
 * - Controls backlight (enabled when playing, disabled when stopped)
 *
 * @param _runStream If true, start playback; if false, stop playback. Default
 * is true.
 * @param _preset The preset number to play (0 = use current preset, >0 =
 * specific preset). Only used when play = true.
 *
 * @note The function does not toggle - it explicitly starts or stops based on
 * the 'play' parameter.
 *
 * @see datamode (global) - Current playback state (STOPPED = not playing)
 * @see stopStream() - Function to stop the current stream
 * @see playStream() - Function to start a stream with a given preset
 * @see showTextOnTFT() - Display text on the TFT screen
 * @see enableBacklight() - Control display backlight
 */
bool runStream(bool _runStream, int16_t _preset = 0) {
  if (_runStream) { // datamode != STOPPED
    dbgprint("Starting stream");

    if (_preset > MAXPRESETS | _preset < 0) {
      return false;
    }

    playSound(false);
    hostreq = true; // Force immediate connection establishment
    // muteflag = false; // sound on
    ini_block.newpreset = _preset;
    playlist_num = 0;
    currentpreset = -1;

    // Display info
    enableBacklight(true);
    showTextOnTFT(1, "Stream started");
    showTextOnTFT(3, "Radio on");

    playSound(true);

    return true;
  } else {
    dbgprint("Stopping stream");

    playSound(false);
    datamode = STOPREQD;

    // Display info
    showTextOnTFT(1, "Stream stopped");
    showTextOnTFT(3, "Radio off");

    enableBacklight(false);

    return true;
  }
}