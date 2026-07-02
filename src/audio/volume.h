#include "VolumeMapper.h"

/** Duration in ms for soft volume fade on play start/stop. */
static const uint32_t VOLUME_FADE_MS = 1000;

static bool volumeFadeActive = false;
static bool volumeFadeIn = true;
static uint32_t volumeFadeStartMs = 0;
static uint32_t volumeFadeDurationMs = VOLUME_FADE_MS;
static uint8_t volumeFadeStartLevel = 0;
static uint8_t volumeFadeCurrent = 0;
static bool volumeFadeStopPending = false;
static bool volumeFadeStartPending = false;

/**
 * @brief Returns the volume level to apply to the VS1053 right now.
 *
 * Uses fade level while a soft fade is active, otherwise reqvol (or 0 when muted).
 *
 * @return uint8_t Output volume 0-100
 */
uint8_t getOutputVolume(void) {
  if (muteflag) {
    return 0;
  }

  if (volumeFadeActive) {
    return volumeFadeCurrent;
  }

  return ini_block.reqvol;
}

/**
 * @brief Returns the mapped volume to program into the VS1053 right now.
 *
 * Mirrors getOutputVolume() (fade- and mute-aware) but maps the user-facing
 * value into the audible chip range. True silence (mute or a fade that is
 * currently at zero) is preserved as chip volume 0.
 *
 * @return uint8_t Chip volume (0 for silence, otherwise VolumeMapper::kChipFloor-100)
 */
uint8_t getChipVolume(void) {
  if (muteflag) {
    return 0;
  }

  if (volumeFadeActive) {
    if (volumeFadeCurrent == 0) {
      return 0;
    }
    return VolumeMapper::mapExternalToChip(volumeFadeCurrent);
  }

  return VolumeMapper::mapExternalToChip(ini_block.reqvol);
}

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
    muteflag = false;               // Mute/unmute: boolean{mute=true, play=false}
    String stationName = metaGetStation();
    showTextOnTFT(3, stationName); // Set screen segment bottom part
  } else {
    muteflag = true;          // Mute/unmute: boolean{mute=true, play=false}
    showTextOnTFT(3, "Mute"); // Set screen segment bottom part
  }
}

/**
 * @brief Completes stream stop after a fade-out (mute, stop playback, backlight off).
 */
static void finishStreamStop(void) {
  playSound(false);
  datamode = STOPREQD;
  enableBacklight(false);
}

/**
 * @brief Advances an in-progress volume fade based on elapsed time.
 *
 * Called periodically from spftask (about every 100 ms).
 */
static void updateVolumeFade(void) {
  if (!volumeFadeActive) {
    return;
  }

  uint32_t elapsed = millis() - volumeFadeStartMs;

  if (volumeFadeIn) {
    const uint8_t target = ini_block.reqvol;
    const uint8_t start = volumeFadeStartLevel;

    if (elapsed >= volumeFadeDurationMs || target <= start) {
      volumeFadeCurrent = target;
      volumeFadeActive = false;
      volumeFadeDurationMs = VOLUME_FADE_MS;
    } else {
      volumeFadeCurrent = start + (uint8_t)((uint32_t)(target - start) * elapsed / volumeFadeDurationMs);
    }
    return;
  }

  const uint8_t start = volumeFadeStartLevel;

  if (elapsed >= volumeFadeDurationMs || start == 0) {
    volumeFadeCurrent = 0;
    volumeFadeActive = false;
    volumeFadeDurationMs = VOLUME_FADE_MS;
    if (volumeFadeStopPending) {
      volumeFadeStopPending = false;
      finishStreamStop();
    }
  } else {
    volumeFadeCurrent = start - (uint8_t)((uint32_t)start * elapsed / volumeFadeDurationMs);
  }
}

/**
 * @brief Starts a soft fade from the current level (or 0) to reqvol.
 */
static void startVolumeFadeIn(void) {
  uint8_t fromLevel = 0;

  if (volumeFadeActive && !volumeFadeIn) {
    fromLevel = volumeFadeCurrent;
  }

  volumeFadeIn = true;
  volumeFadeStopPending = false;
  volumeFadeStartMs = millis();
  volumeFadeStartLevel = fromLevel;
  volumeFadeCurrent = fromLevel;
  volumeFadeActive = (ini_block.reqvol > fromLevel);
}

/**
 * @brief Starts a long fade-in for alarm wake (from silence to reqvol).
 *
 * @param durationSec Fade duration in seconds
 */
void startAlarmVolumeFade(uint16_t durationSec) {
  volumeFadeDurationMs = (uint32_t)durationSec * 1000UL;
  if (volumeFadeDurationMs < 1000UL) {
    volumeFadeDurationMs = 1000UL;
  }
  volumeFadeIn = true;
  volumeFadeStopPending = false;
  volumeFadeStartMs = millis();
  volumeFadeStartLevel = 0;
  volumeFadeCurrent = 0;
  volumeFadeActive = true;
  muteflag = false;
  volumeFadeStartPending = false;
}

/**
 * @brief Starts a soft fade from the current level down to 0.
 *
 * @param stopAfter If true, stop the stream when the fade reaches zero.
 */
static void startVolumeFadeOut(bool stopAfter) {
  const uint8_t startLevel = getOutputVolume();

  volumeFadeIn = false;
  volumeFadeStopPending = stopAfter;
  volumeFadeStartPending = false;
  volumeFadeStartMs = millis();
  volumeFadeStartLevel = startLevel;
  volumeFadeCurrent = startLevel;

  if (startLevel == 0) {
    volumeFadeActive = false;
    if (stopAfter) {
      finishStreamStop();
    }
    return;
  }

  volumeFadeActive = true;
}

/**
 * @brief Start queued play fade once decoder start is confirmed.
 *
 * Called from the play task when QSTARTSONG is processed. This avoids enabling
 * audio output before valid stream data is available.
 */
void beginStreamAudioOnDecoderStart(void) {
  if (vs1053Available && vs1053player) {
    vs1053player->setOutputEnabled(true);
  }

  if (volumeFadeStartPending) {
    volumeFadeStartPending = false;
    playSound(true);
    startVolumeFadeIn();
    return;
  }

  if (datamode == DATA || datamode == PLAYLISTDATA) {
    playSound(true);
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

/** Volume granularity for user-initiated (online/encoder) changes. */
static const uint8_t VOLUME_STEP = 5;

/**
 * @brief Snap a volume value to the nearest VOLUME_STEP multiple (0-100).
 *
 * Keeps every user-facing volume change quantized to 5-step increments,
 * regardless of the client (Web UI, companion app, or serial/API).
 *
 * @param volume Raw volume value 0-100
 * @return uint8_t Volume rounded to the nearest multiple of VOLUME_STEP
 */
static uint8_t roundVolumeToStep(uint8_t volume) {
  uint16_t snapped = ((uint16_t)volume + (VOLUME_STEP / 2)) / VOLUME_STEP * VOLUME_STEP;
  if (snapped > 100) {
    snapped = 100;
  }
  return (uint8_t)snapped;
}

/**
 * @brief Sets the absolute volume level (snapped to 5-step increments).
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

  ini_block.reqvol = roundVolumeToStep(volume);
  nvsRequestSettingsSave(); // Persist the new volume shortly after the change
  return true;
}

/**
 * @brief Adjusts the volume by a specified relative amount (snapped to 5-step increments).
 *
 * Adds the given delta to the current volume. The result is constrained
 * to the range [0, 100] and rounded to the nearest VOLUME_STEP multiple.
 *
 * @param delta The value to add (positive to increase, negative to decrease)
 * @return uint8_t The new volume level
 */
uint8_t adjustVolume(int8_t delta) {
  int16_t newVolume = (int16_t)ini_block.reqvol + delta;

  ini_block.reqvol = roundVolumeToStep((uint8_t)constrain(newVolume, 0, 100));
  nvsRequestSettingsSave(); // Persist the new volume shortly after the change

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
    volumeFadeStartPending = true;
    hostreq = true; // Force immediate connection establishment
    // muteflag = false; // sound on
    ini_block.newpreset = _preset;
    playlist_num = 0;
    currentpreset = -1;

    // Display info
    enableBacklight(true);
#if !defined(BLUETFT)
    showTextOnTFT(1, "Stream started");
#endif
    showTextOnTFT(3, "Radio on");

    return true;
  } else {
    dbgprint("Stopping stream (soft fade)");

#if !defined(BLUETFT)
    showTextOnTFT(1, "Stream stopped");
#endif
    showTextOnTFT(3, "Radio off");

    if (datamode != STOPPED || volumeFadeActive || volumeFadeStartPending) {
      startVolumeFadeOut(true);
    } else {
      finishStreamStop();
    }

    return true;
  }
}