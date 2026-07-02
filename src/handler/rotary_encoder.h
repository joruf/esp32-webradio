#if defined(BLUETFT)
#include "display/dsp_dashboard.h"
#endif

/**
 * @brief Show a preset station name on TFT segment 2 (station row).
 *
 * @param preset Preset index in NVS
 */
static void encShowPresetNameOnTft(int8_t preset) {
  String tmp = nvsGetHostFromPref(preset);
  int16_t inx;

  if (tmp == "") {
    return;
  }

  inx = tmp.indexOf("#");
  if (inx > 0) {
    tmp.remove(0, inx + 1);
  }
  chomp(tmp);
  showTextOnTFT(2, tmp.c_str());
}

/**
 * @brief Rotary encoder handling: volume, preset selection, and track
 * selection.
 *
 * This function is called regularly to process encoder clicks, double-clicks,
 * rotations, and inactivity timeouts. It controls power toggling, preset
 * selection, volume adjustment, and (optionally) track selection from SD card.
 *
 * @note The original logic is kept exactly as in the working version to ensure
 *       continuous volume control after power toggle.
 */
void checkRotaryEncoder() {
  static int8_t enc_preset = 0;
  String tmp;
  int16_t inx;

  // Inactivity timer: return to VOLUME mode after 4 seconds
  if (enc_menu_mode != VOLUME) {
    if (enc_inactivity > 40) {
      enc_inactivity = 0;
      enc_menu_mode = VOLUME;
      dbgprint("Encoder mode back to VOLUME");
#if defined(BLUETFT)
      dspDashboardInvalidateStation();
#endif
      String stationName = metaGetStation();
      if (stationName.length() > 0) {
        showTextOnTFT(2, stationName.c_str());
      } else {
        showTextOnTFT(2, "");
      }
      showTextOnTFT(3, "");
    }
  }

  // Activate backlight on any activity
  if (singleclick || longclick || doubleclick || rotationcount != 0) {
    enableBacklight(true);
    bltimer = 0;
  } else {
    return;
  }

  // --- Double click: enter preset selection mode ---
  if (doubleclick) {
    dbgprint("Double click detected - Enter preset selection mode");
    doubleclick = false;
    enc_menu_mode = PRESET;
    enc_preset = ini_block.newpreset;
#if defined(BLUETFT)
    dspDashboardInvalidateStation();
#endif
    encShowPresetNameOnTft(enc_preset);
    showTextOnTFT(3, msgPresetSelectTft);
    return;
  }

  // --- Long click: enter preset selection mode (process FIRST) ---
  if (longclick) {
    dbgprint("Long click detected - Enter preset selection mode");
    longclick = false; // Clear immediately
    enc_menu_mode = PRESET;
    enc_preset = ini_block.newpreset;
#if defined(BLUETFT)
    dspDashboardInvalidateStation();
#endif
    encShowPresetNameOnTft(enc_preset);
    showTextOnTFT(3, msgPresetSelectTft);
    return; // Exit, don't process single click in same cycle
  }

  // --- Single click: depends on current mode (only if longclick was false) ---
  if (singleclick) {
    singleclick = false;
    if (enc_menu_mode == VOLUME) {
      // Power toggle
      dbgprint("Single click - Power toggle");
      if (datamode != STOPPED) {
        runStream(false);
      } else {

        if (enc_preset < 0 || enc_preset > MAXPRESETS) {
          enc_preset = 0;
        }
        ini_block.newpreset = enc_preset;
        runStream(true, currentpreset);
        playlist_num = 0;
        currentpreset = -1;
      }
    } else if (enc_menu_mode == PRESET) {
      // Confirm selected preset and return to volume mode
      dbgprint("Single click - Confirm preset");
      ini_block.newpreset = enc_preset;
      playlist_num = 0;
      currentpreset = -1;
      enc_menu_mode = VOLUME;
      encShowPresetNameOnTft(enc_preset);
      showTextOnTFT(3, "Sender gewählt");
#if defined(BLUETFT)
      dspDashboardInvalidateStation();
#endif
      if (datamode != STOPPED) {
        // Stop current stream first; mp3loop() loads the new host once STOPPED
        datamode = STOPREQD;
      } else {
        runStream(true, enc_preset);
      }
    }
    // TRACK mode is not used
  }

  // --- Rotation handling ---
  if (rotationcount == 0) {
    return;
  }

  switch (enc_menu_mode) {
    case VOLUME:
      ini_block.reqvol = constrain(ini_block.reqvol + rotationcount, 0, 100);
      muteflag = false;
      nvsRequestSettingsSave(); // Persist the new volume shortly after the change
      break;

    case PRESET:
      enc_preset += rotationcount;
      if (enc_preset < 0) {
        enc_preset = MAXPRESETS;
      }
      tmp = nvsGetHostFromPref(enc_preset);
      if (tmp == "") {
        enc_preset = 0;
        tmp = nvsGetHostFromPref(enc_preset);
      }
      inx = tmp.indexOf("#");
      if (inx > 0) {
        tmp.remove(0, inx + 1);
      }
      chomp(tmp);
      showTextOnTFT(2, tmp.c_str());
      break;

    default:
      break;
  }
  rotationcount = 0;
}
