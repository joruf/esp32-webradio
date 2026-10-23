#if defined(RADIO_DASHBOARD_TFT)
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
#if defined(RADIO_DASHBOARD_TFT)
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
#if defined(RADIO_DASHBOARD_TFT)
    dspDashboardInvalidateStation();
#endif
    encShowPresetNameOnTft(enc_preset);
    showTextOnTFT(3, msgPresetSelectTft);
    return;
  }

  // --- Long click: SD track mode when available, otherwise preset selection ---
  if (longclick) {
    longclick = false;
    if (SD_okay && hardwareProfileTrackEncoder()) {
      dbgprint("Long click detected - Enter SD track selection mode");
      enc_menu_mode = TRACK;
      showTextOnTFT(3, "SD track");
      if (SD_currentnode.length() == 0 || SD_currentnode == "0") {
        int nl = SD_nodelist.indexOf("\n");
        if (nl > 0) {
          SD_currentnode = SD_nodelist.substring(0, nl);
        } else {
          SD_currentnode = selectnextSDnode("0", +1);
        }
      }
      {
        String name = getSDfilename(SD_currentnode);
        int slash = name.lastIndexOf('/');
        if (slash >= 0) {
          name = name.substring(slash + 1);
        }
        showTextOnTFT(2, name.c_str());
      }
      return;
    }
    dbgprint("Long click detected - Enter preset selection mode");
    enc_menu_mode = PRESET;
    enc_preset = ini_block.newpreset;
#if defined(RADIO_DASHBOARD_TFT)
    dspDashboardInvalidateStation();
#endif
    encShowPresetNameOnTft(enc_preset);
    showTextOnTFT(3, msgPresetSelectTft);
    return;
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
#if defined(RADIO_DASHBOARD_TFT)
      dspDashboardInvalidateStation();
#endif
      if (datamode != STOPPED) {
        // Stop current stream first; mp3loop() loads the new host once STOPPED
        datamode = STOPREQD;
      } else {
        runStream(true, enc_preset);
      }
    } else if (enc_menu_mode == TRACK) {
      dbgprint("Single click - Confirm SD track");
      String fname = getSDfilename(SD_currentnode);
      if (fname.length() > 0) {
        host = String("localhost") + fname;
        hostreq = true;
        if (datamode != STOPPED) {
          datamode = STOPREQD;
        }
      }
      enc_menu_mode = VOLUME;
      showTextOnTFT(3, "SD playing");
    }
  }

  // --- Rotation handling ---
  if (rotationcount == 0) {
    return;
  }

  // Per-device wiring / mount may reverse CLK/DT sense.
  int16_t steps = rotationcount;
  if (radioFeatures.encoder_invert) {
    steps = (int16_t)(-steps);
  }

  switch (enc_menu_mode) {
    case VOLUME:
      // Each detent changes the user-facing volume by 5 steps (0-100 range).
      ini_block.reqvol = constrain(ini_block.reqvol + steps * 5, 0, 100);
      muteflag = false;
      nvsRequestSettingsSave(); // Persist the new volume shortly after the change
      break;

    case PRESET:
      enc_preset += steps;
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

    case TRACK:
      if (SD_okay) {
        SD_currentnode = selectnextSDnode(SD_currentnode, steps > 0 ? +1 : -1);
        tmp = getSDfilename(SD_currentnode);
        inx = tmp.lastIndexOf('/');
        if (inx >= 0) {
          tmp = tmp.substring(inx + 1);
        }
        showTextOnTFT(2, tmp.c_str());
      }
      break;

    default:
      break;
  }
  rotationcount = 0;
}
