/**
 * @file hardware_profile.h
 * @brief Selectable hardware configuration templates (pins + feature defaults).
 *
 * Profiles can be applied from the Web UI. Each PlatformIO binary still ships with
 * a compile-time default profile, but the user may switch templates at runtime.
 * A reboot is required after applying a profile so pins and SD backend take effect.
 */

#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H

#include <ArduinoJson.h>
#include <string.h>

#include "config/nvs.h"

#ifndef HW_PROFILE_DEFAULT
#define HW_PROFILE_DEFAULT "wroom_tft"
#endif

/** Profile id: ESP32-WROOM + VS1053 + rotary encoder + TFT (current radio). */
#define HW_PROFILE_WROOM_TFT "wroom_tft"
/** Profile id: ESP32-CAM (no camera) + VS1053 + rotary encoder + onboard SD_MMC. */
#define HW_PROFILE_CAM_HEADLESS "cam_headless"
/** Profile id: Waveshare ESP32-S3-Touch-LCD-2.8B + VS1053 (no rotary encoder). */
#define HW_PROFILE_S3_TOUCH_28B "s3_touch_28b"

struct HwPinEntry {
  const char *key;
  int8_t value;
};

struct HwGpioEntry {
  int pin;
  const char *command;
};

struct HardwareProfile {
  const char *id;
  const char *label;
  const char *description;
  bool display_ui;
  bool sd_enabled;
  bool track_encoder;
  /** "none", "spi" (VSPI + CS) or "mmc" (ESP32-CAM onboard 1-bit). */
  const char *sd_backend;
  const HwPinEntry *pins;
  size_t pin_count;
  const HwGpioEntry *gpio;
  size_t gpio_count;
};

static const HwPinEntry kPinsWroomTft[] = {
    {"pin_enc_clk", 25},  {"pin_enc_dt", 26},   {"pin_enc_sw", 27},
    {"pin_ir", 35},       {"pin_sd_cs", 21},    {"pin_tft_cs", 15},
    {"pin_tft_dc", 2},    {"pin_vs_cs", 5},     {"pin_vs_dcs", 32},
    {"pin_vs_dreq", 4},   {"pin_tft_scl", -1},  {"pin_tft_sda", -1},
    {"pin_tft_bl", 33},   {"pin_tft_blx", -1},  {"pin_shutdown", -1},
    {"pin_shutdownx", -1},{"pin_spi_sck", 18},  {"pin_spi_miso", 19},
    {"pin_spi_mosi", 23},
};

static const HwGpioEntry kGpioWroomTft[] = {
    {0, "uppreset = 1"},
    {12, "upvolume = 2"},
    {13, "downvolume = 2"},
    {14, "stop"},
    {17, "resume"},
    {34, "station = icecast.omroep.nl:80/radio1-bb-mp3"},
};

static const HwPinEntry kPinsCamHeadless[] = {
    {"pin_enc_clk", 25},  {"pin_enc_dt", 26},   {"pin_enc_sw", 27},
    {"pin_ir", -1},       {"pin_sd_cs", -1},    {"pin_tft_cs", -1},
    {"pin_tft_dc", -1},   {"pin_vs_cs", 5},     {"pin_vs_dcs", 32},
    {"pin_vs_dreq", 4},   {"pin_tft_scl", -1},  {"pin_tft_sda", -1},
    {"pin_tft_bl", -1},   {"pin_tft_blx", -1},  {"pin_shutdown", -1},
    {"pin_shutdownx", -1},{"pin_spi_sck", 18},  {"pin_spi_miso", 19},
    {"pin_spi_mosi", 23},
};

/** Waveshare S3 Touch 2.8B pin map (USB-aware).
 *  GPIO4 has an onboard battery divider - do not use for VS1053 CS/DCS/DREQ.
 *  With MCP: CS/DCS on expander; MISO=GPIO0; DREQ=GPIO16; GT911 INT only at reset.
 *  Without MCP: CS=0, DCS=16, DREQ=20 (D+), no MISO (write-only). SCK=43 MOSI=44.
 *  Encoder off (0 used as CS or MISO). */
static const HwPinEntry kPinsS3Touch28b[] = {
#if defined(USE_MCP23017_VS1053)
    {"pin_enc_clk", -1},  {"pin_enc_dt", -1},   {"pin_enc_sw", -1},
    {"pin_vs_cs", -20},   {"pin_vs_dcs", -21},
    {"pin_vs_dreq", 16},  {"pin_spi_miso", 0},
#else
    {"pin_enc_clk", -1},  {"pin_enc_dt", -1},   {"pin_enc_sw", -1},
    {"pin_vs_cs", 0},     {"pin_vs_dcs", 16},
    {"pin_vs_dreq", 20},  {"pin_spi_miso", -1},
#endif
    {"pin_ir", -1},       {"pin_sd_cs", -1},    {"pin_tft_cs", -1},
    {"pin_tft_dc", -1},
    {"pin_tft_scl", -1},  {"pin_tft_sda", -1},
    {"pin_tft_bl", -1},   {"pin_tft_blx", -1},  {"pin_shutdown", -1},
    {"pin_shutdownx", -1},{"pin_spi_sck", 43},
    {"pin_spi_mosi", 44},
};

static const HardwareProfile kHardwareProfiles[] = {
    {
        HW_PROFILE_WROOM_TFT,
        "ESP32-WROOM + TFT",
        "WROOM, VS1053, rotary encoder, colour TFT and SPI SD card.",
        true,
        true,
        false,
        "spi",
        kPinsWroomTft,
        sizeof(kPinsWroomTft) / sizeof(kPinsWroomTft[0]),
        kGpioWroomTft,
        sizeof(kGpioWroomTft) / sizeof(kGpioWroomTft[0]),
    },
    {
        HW_PROFILE_CAM_HEADLESS,
        "ESP32-CAM (no camera)",
        "CAM without camera, VS1053, rotary encoder, onboard SD (SD_MMC). No display. Long-click encoder for SD tracks.",
        false,
        true,
        true,
        "mmc",
        kPinsCamHeadless,
        sizeof(kPinsCamHeadless) / sizeof(kPinsCamHeadless[0]),
        /* No factory GPIOs: CAM SD uses 2/14/15; 12/13/16/17 are unsafe. */
        nullptr,
        0,
    },
    {
        HW_PROFILE_S3_TOUCH_28B,
        "ESP32-S3 Touch LCD 2.8B",
        "Waveshare S3 Touch 2.8B, ST7701, VS1053 without USB pins: MCP CS/DCS + MISO0/DREQ4, or no-MCP CS0/DCS4/DREQ16 write-only. Touch swipe volume.",
        true,  // display_ui
        false, // sd_enabled
        false, // track_encoder (0/4 used for VS1053)
        "none",
        kPinsS3Touch28b,
        sizeof(kPinsS3Touch28b) / sizeof(kPinsS3Touch28b[0]),
        nullptr,
        0,
    },
};

/**
 * @brief Number of built-in hardware profiles.
 */
inline size_t hardwareProfileCount(void) {
  return sizeof(kHardwareProfiles) / sizeof(kHardwareProfiles[0]);
}

/**
 * @brief Look up a hardware profile by id.
 *
 * @param id Profile id string
 * @return const HardwareProfile* Profile or NULL
 */
inline const HardwareProfile *findHardwareProfile(const char *id) {
  if (id == NULL || id[0] == '\0') {
    return NULL;
  }
  for (size_t i = 0; i < hardwareProfileCount(); i++) {
    if (strcmp(kHardwareProfiles[i].id, id) == 0) {
      return &kHardwareProfiles[i];
    }
  }
  return NULL;
}

/**
 * @brief Return the compile-time default profile id for this firmware binary.
 */
inline const char *hardwareProfileBuildDefaultId(void) { return HW_PROFILE_DEFAULT; }

/**
 * @brief Return the active profile id from NVS, or the build default.
 */
inline String hardwareProfileActiveId(void) {
  if (nvsKeyExists("hw_profile")) {
    String id = nvsGetValue("hw_profile");
    if (findHardwareProfile(id.c_str()) != NULL) {
      return id;
    }
  }
  return String(HW_PROFILE_DEFAULT);
}

/**
 * @brief Return the active hardware profile struct.
 */
inline const HardwareProfile *hardwareProfileActive(void) {
  const HardwareProfile *p = findHardwareProfile(hardwareProfileActiveId().c_str());
  if (p == NULL) {
    p = findHardwareProfile(HW_PROFILE_DEFAULT);
  }
  return p;
}

/**
 * @brief True when the Display Settings page should be shown.
 */
inline bool hardwareProfileShowsDisplayUi(void) {
  const HardwareProfile *p = hardwareProfileActive();
  return p != NULL && p->display_ui;
}

/**
 * @brief True when SD track selection via encoder should be enabled.
 */
inline bool hardwareProfileTrackEncoder(void) {
  const HardwareProfile *p = hardwareProfileActive();
  return p != NULL && p->track_encoder;
}

/**
 * @brief True when SD should be mounted for the active profile.
 */
inline bool hardwareProfileSdEnabled(void) {
  const HardwareProfile *p = hardwareProfileActive();
  return p != NULL && p->sd_enabled;
}

/**
 * @brief SD backend for the active profile ("none", "spi", "mmc").
 */
inline const char *hardwareProfileSdBackend(void) {
  const HardwareProfile *p = hardwareProfileActive();
  if (p == NULL || p->sd_backend == NULL) {
    return "none";
  }
  return p->sd_backend;
}

/**
 * @brief Persist a hardware profile (pins, gpio factory list, flags) to NVS.
 *
 * Caller should reboot afterwards so SPI/TFT/SD init uses the new pins.
 *
 * @param id Profile id
 * @return true if the profile was found and written
 */
inline bool applyHardwareProfile(const char *id) {
  const HardwareProfile *p = findHardwareProfile(id);
  if (p == NULL) {
    return false;
  }

  nvsOpen();
  nvsAddValue("hw_profile", String(p->id));
  nvsAddValue("feat_sd_en", String(p->sd_enabled ? 1 : 0));
  nvsAddValue("feat_sd_be", String(p->sd_backend));
  nvsAddValue("feat_track_enc", String(p->track_encoder ? 1 : 0));
  nvsAddValue("feat_disp_ui", String(p->display_ui ? 1 : 0));

  for (size_t i = 0; i < p->pin_count; i++) {
    nvsAddValue(p->pins[i].key, String((int)p->pins[i].value));
  }

  char key[16];
  for (int i = 0; i < 100; i++) {
    sprintf(key, "gpio_%02d", i);
    if (nvsKeyExists(key)) {
      nvs_erase_key(nvshandle, key);
    }
  }
  for (size_t i = 0; i < p->gpio_count; i++) {
    sprintf(key, "gpio_%02d", p->gpio[i].pin);
    nvsAddValue(key, String(p->gpio[i].command));
  }

  nvsCommit();
  return true;
}

/**
 * @brief Escape a short ASCII string for JSON string values.
 */
inline String hwProfileJsonEscape(const char *text) {
  String out;
  if (text == NULL) {
    return out;
  }
  for (const char *p = text; *p; p++) {
    if (*p == '"' || *p == '\\') {
      out += '\\';
    }
    out += *p;
  }
  return out;
}

/**
 * @brief Append hardware profile list + active id to a JSON object builder string.
 *
 * @param json In/out JSON body being built (no trailing brace assumed by caller)
 */
inline void appendHardwareProfileStatusJson(String &json) {
  const String active = hardwareProfileActiveId();
  json += "\"hardware_profile\":\"" + active + "\",";
  json += "\"hardware_profile_default\":\"" + String(HW_PROFILE_DEFAULT) + "\",";
  json += "\"hardware_display_ui\":" + String(hardwareProfileShowsDisplayUi() ? "true" : "false") + ",";
  json += "\"hardware_sd_enabled\":" + String(hardwareProfileSdEnabled() ? "true" : "false") + ",";
  json += "\"hardware_sd_backend\":\"" + String(hardwareProfileSdBackend()) + "\",";
  json += "\"hardware_track_encoder\":" + String(hardwareProfileTrackEncoder() ? "true" : "false") + ",";
  json += "\"hardware_profiles\":[";
  for (size_t i = 0; i < hardwareProfileCount(); i++) {
    if (i > 0) {
      json += ",";
    }
    const HardwareProfile &pr = kHardwareProfiles[i];
    json += "{";
    json += "\"id\":\"" + String(pr.id) + "\",";
    json += "\"label\":\"" + hwProfileJsonEscape(pr.label) + "\",";
    json += "\"description\":\"" + hwProfileJsonEscape(pr.description) + "\",";
    json += "\"display_ui\":" + String(pr.display_ui ? "true" : "false") + ",";
    json += "\"sd_enabled\":" + String(pr.sd_enabled ? "true" : "false") + ",";
    json += "\"sd_backend\":\"" + String(pr.sd_backend) + "\",";
    json += "\"track_encoder\":" + String(pr.track_encoder ? "true" : "false");
    json += "}";
  }
  json += "]";
}

/**
 * @brief Build a standalone JSON document for getHardwareProfiles.
 */
inline String buildHardwareProfilesJson(void) {
  String json = "{";
  appendHardwareProfileStatusJson(json);
  json += "}";
  return json;
}

/**
 * @brief Default pin value for an IO key for the build-default profile.
 *
 * Used when an NVS pin key is missing so CAM builds boot with CAM pins
 * even before the user applies a profile once.
 *
 * @param key NVS pin key (e.g. pin_tft_cs)
 * @param fallback Compile-time fallback
 * @return int8_t Default pin
 */
inline int8_t hardwareProfileDefaultPin(const char *key, int8_t fallback) {
  const HardwareProfile *p = findHardwareProfile(HW_PROFILE_DEFAULT);
  if (p == NULL) {
    return fallback;
  }
  for (size_t i = 0; i < p->pin_count; i++) {
    if (strcmp(p->pins[i].key, key) == 0) {
      return p->pins[i].value;
    }
  }
  return fallback;
}

#endif
