/**
 * @file alarm_fallback.h
 * @brief Offline wake-up tones via VS1053 sine test (no SD / SPIFFS samples).
 *
 * Used only during an active alarm session after the stream watchdog has given
 * up reconnecting (no network / dead stream). Patterns are soft on/off and
 * frequency timing only; the classic SDI sine test requires stop before each
 * new frequency so sweeps and peeps actually change pitch.
 */

#ifndef ALARM_FALLBACK_H
#define ALARM_FALLBACK_H

#include "config/features_config.h"
#include "globals/globals.h"
#include "system/event_log.h"
#include "system/spi.h"

/** True between scheduled alarm start and stop (or explicit cancel). */
static bool alarmSessionActive = false;
static bool alarmFallbackPlaying = false;
static uint32_t alarmFallbackNextMs = 0;
static uint8_t alarmFallbackPhase = 0;
static uint8_t alarmFallbackStep = 0;
static uint8_t alarmFallbackToneN = 0x44;
static uint8_t alarmFallbackKind = 0;

enum : uint8_t {
  AF_BEEP = 0,
  AF_CHIRP = 1,
  AF_BIRDS = 2,
  AF_WATER = 3,
  AF_CASCADE = 4,
  AF_ARCADE = 5
};

/** One sine-test note: n=0 is a rest; otherwise VS1053 startSineTest parameter. */
struct AfMelodyNote {
  uint8_t n;
  uint16_t ms;
} __attribute__((packed));

/**
 * @brief Map a stable sound ID string to an internal pattern kind.
 */
static uint8_t parseAlarmFallbackKind(const String &sound) {
  if (sound.equalsIgnoreCase("chirp")) {
    return AF_CHIRP;
  }
  if (sound.equalsIgnoreCase("birds")) {
    return AF_BIRDS;
  }
  if (sound.equalsIgnoreCase("water")) {
    return AF_WATER;
  }
  if (sound.equalsIgnoreCase("cascade")) {
    return AF_CASCADE;
  }
  if (sound.equalsIgnoreCase("arcade")) {
    return AF_ARCADE;
  }
  return AF_BEEP;
}

/**
 * @brief Short TFT / log label for the selected sound.
 */
static const char *alarmFallbackLabel(uint8_t kind) {
  switch (kind) {
  case AF_CHIRP:
    return "Chirp";
  case AF_BIRDS:
    return "Birds";
  case AF_WATER:
    return "Water";
  case AF_CASCADE:
    return "Cascade";
  case AF_ARCADE:
    return "Arcade";
  default:
    return "Beep";
  }
}

/**
 * @brief Configured gap between pattern cycles (ms), never below 500 ms.
 */
static uint32_t alarmFallbackGapMs(void) {
  uint32_t gapMs = (uint32_t)radioFeatures.alarm_fallback_interval_sec * 1000UL;
  if (gapMs < 500UL) {
    gapMs = 500UL;
  }
  return gapMs;
}

/**
 * @brief Stop any active sine, then start a new frequency.
 *
 * The classic VS1053 SDI sine test ignores a new start sequence while a tone
 * is already running; without this exit/restart pair, chirps stay stuck on
 * the first frequency and sound identical to a fixed beep.
 */
static void alarmFallbackSetSine(uint8_t n) {
  vs1053player->stopSineTest();
  vs1053player->startSineTest(n);
  alarmFallbackToneN = n;
}

static void alarmFallbackSilence(void) {
  vs1053player->stopSineTest();
}

bool isAlarmSessionActive(void) {
  return alarmSessionActive;
}

bool isAlarmFallbackPlaying(void) {
  return alarmFallbackPlaying;
}

void setAlarmSessionActive(bool active) {
  alarmSessionActive = active;
}

/**
 * @brief Stop the offline wake tone and leave the decoder ready for streams.
 */
void stopAlarmFallback(void) {
  if (!alarmFallbackPlaying) {
    return;
  }
  alarmFallbackPlaying = false;
  alarmFallbackPhase = 0;
  alarmFallbackStep = 0;
  alarmFallbackNextMs = 0;
  if (vs1053player && vs1053Available) {
    if (claimSPI("alrmfb_stop")) {
      vs1053player->stopSineTest();
      releaseSPI();
    }
  }
  dbgprint("Alarm fallback tone stopped");
}

/**
 * @brief Begin looping the selected wake pattern after stream failure / Test.
 *
 * Plays immediately at the current audible volume (no fade-in). Stream alarm
 * fade remains separate and only applies to radio playback.
 */
void startAlarmFallback(void) {
  if (!alarmSessionActive || !vs1053Available || !vs1053player) {
    return;
  }
  if (alarmFallbackPlaying) {
    return;
  }

  stop_mp3client();
  hostreq = false;
  chunked = false;
  datacount = 0;
  outqp = outchunk.buf;
  metaint = 0;
  if (dataqueue) {
    xQueueReset(dataqueue);
  }
  datamode = STOPPED;
  localfile = false;

  // Skip any stream/alarm fade so the sine tone is audible at once.
  cancelVolumeFade();
  muteflag = false;

  alarmFallbackKind = parseAlarmFallbackKind(radioFeatures.alarm_fallback_sound);
  alarmFallbackPlaying = true;
  alarmFallbackPhase = 0;
  alarmFallbackStep = 0;
  alarmFallbackToneN = 0x44;
  // Poll applies the first tone immediately so the selection is never overridden
  // by a hard-coded mid beep.
  alarmFallbackNextMs = millis();

  if (claimSPI("alrmfb_start")) {
    vs1053player->setOutputEnabled(true);
    vs1053player->setVolume(getChipVolume());
    releaseSPI();
  }

  const char *sound = radioFeatures.alarm_fallback_sound.c_str();
  eventLogRecordWarning((String("Alarm fallback: ") + sound).c_str());
  showTextOnTFT(1, "Alarm tone");
  showTextOnTFT(3, alarmFallbackLabel(alarmFallbackKind));
  dbgprint("Alarm fallback started (%s) vol=%u", sound, (unsigned)getChipVolume());
}

/**
 * @brief Classic mid-tone on/off beep, then configured interval.
 */
static void pollAlarmFallbackBeep(uint32_t now) {
  if (alarmFallbackPhase == 0) {
    alarmFallbackSetSine(0x44); // ~1 kHz mid beep
    alarmFallbackPhase = 1;
    alarmFallbackNextMs = now + 380;
  } else if (alarmFallbackPhase == 1) {
    alarmFallbackSilence();
    alarmFallbackPhase = 2;
    alarmFallbackNextMs = now + 160;
  } else {
    alarmFallbackPhase = 0;
    alarmFallbackNextMs = now + alarmFallbackGapMs();
  }
}

/**
 * @brief Clear rising frequency sweep (must sound different from beep).
 */
static void pollAlarmFallbackChirp(uint32_t now) {
  // FsIdx=1 (48 kHz) with rising S: audible upward glide, then gap.
  static const uint8_t kChirpNs[] = {0x24, 0x2C, 0x34, 0x3C, 0x44, 0x4C, 0x54, 0x5C, 0x64};
  const uint8_t count = (uint8_t)(sizeof(kChirpNs) / sizeof(kChirpNs[0]));

  if (alarmFallbackPhase == 0) {
    if (alarmFallbackStep < count) {
      alarmFallbackSetSine(kChirpNs[alarmFallbackStep]);
      alarmFallbackStep++;
      alarmFallbackNextMs = now + 55;
    } else {
      alarmFallbackSilence();
      alarmFallbackPhase = 1;
      alarmFallbackStep = 0;
      alarmFallbackNextMs = now + 120;
    }
  } else {
    alarmFallbackPhase = 0;
    alarmFallbackStep = 0;
    alarmFallbackNextMs = now + alarmFallbackGapMs();
  }
}

/**
 * @brief Short high peeps with quiet gaps (pseudo-random from millis).
 */
static void pollAlarmFallbackBirds(uint32_t now) {
  static const uint8_t kBirdNs[] = {0x5C, 0x64, 0x6C, 0x74, 0x68, 0x70};
  const uint8_t count = (uint8_t)(sizeof(kBirdNs) / sizeof(kBirdNs[0]));

  if (alarmFallbackPhase == 0) {
    // A few peeps per cycle, then the configured interval.
    if (alarmFallbackStep >= 4) {
      alarmFallbackSilence();
      alarmFallbackPhase = 1;
      alarmFallbackStep = 0;
      alarmFallbackNextMs = now + alarmFallbackGapMs();
      return;
    }
    uint8_t pick = (uint8_t)((now ^ (millis() >> 3) ^ (alarmFallbackStep * 17U)) % count);
    alarmFallbackSetSine(kBirdNs[pick]);
    alarmFallbackPhase = 2;
    // Short peep length varies slightly.
    alarmFallbackNextMs = now + 45 + (uint32_t)((now >> 2) % 40U);
  } else if (alarmFallbackPhase == 2) {
    alarmFallbackSilence();
    alarmFallbackPhase = 0;
    alarmFallbackStep++;
    // Quiet gap between peeps (soft, not machine-gun).
    alarmFallbackNextMs = now + 180 + (uint32_t)((now >> 4) % 220U);
  } else {
    alarmFallbackPhase = 0;
    alarmFallbackStep = 0;
    alarmFallbackNextMs = now + alarmFallbackGapMs();
  }
}

/**
 * @brief Rapid soft frequency wobble / bubbling brook.
 */
static void pollAlarmFallbackWater(uint32_t now) {
  // Small steps around a gentle mid-low band; short on/off for bubble texture.
  static const uint8_t kWaterNs[] = {0x28, 0x2A, 0x2C, 0x2E, 0x30, 0x2E, 0x2C, 0x2A,
                                    0x2C, 0x32, 0x2E, 0x2A, 0x28, 0x2C, 0x30, 0x2C};
  const uint8_t count = (uint8_t)(sizeof(kWaterNs) / sizeof(kWaterNs[0]));

  if (alarmFallbackPhase == 0) {
    if (alarmFallbackStep >= 48) {
      // One bubbling phrase, then the repeat interval.
      alarmFallbackSilence();
      alarmFallbackPhase = 1;
      alarmFallbackStep = 0;
      alarmFallbackNextMs = now + alarmFallbackGapMs();
      return;
    }
    alarmFallbackSetSine(kWaterNs[alarmFallbackStep % count]);
    alarmFallbackPhase = 2;
    alarmFallbackNextMs = now + 28;
  } else if (alarmFallbackPhase == 2) {
    // Brief hush between micro-bubbles keeps it soft, not a continuous siren.
    alarmFallbackSilence();
    alarmFallbackPhase = 0;
    alarmFallbackStep++;
    alarmFallbackNextMs = now + 12;
  } else {
    alarmFallbackPhase = 0;
    alarmFallbackStep = 0;
    alarmFallbackNextMs = now + alarmFallbackGapMs();
  }
}

/**
 * @brief Gentle descending multi-tone pulse (soft cascade).
 */
static void pollAlarmFallbackCascade(uint32_t now) {
  static const uint8_t kCascadeNs[] = {0x58, 0x50, 0x48, 0x40, 0x38, 0x30};
  const uint8_t count = (uint8_t)(sizeof(kCascadeNs) / sizeof(kCascadeNs[0]));

  if (alarmFallbackPhase == 0) {
    if (alarmFallbackStep < count) {
      alarmFallbackSetSine(kCascadeNs[alarmFallbackStep]);
      alarmFallbackPhase = 2;
      alarmFallbackNextMs = now + 140;
    } else {
      alarmFallbackSilence();
      alarmFallbackPhase = 1;
      alarmFallbackStep = 0;
      alarmFallbackNextMs = now + 200;
    }
  } else if (alarmFallbackPhase == 2) {
    alarmFallbackSilence();
    alarmFallbackPhase = 0;
    alarmFallbackStep++;
    alarmFallbackNextMs = now + 70;
  } else {
    alarmFallbackPhase = 0;
    alarmFallbackStep = 0;
    alarmFallbackNextMs = now + alarmFallbackGapMs();
  }
}

/*
 * Arcade uses mid-high VS1053 sine Ns in the same audible band as cascade/chirp.
 * Rests (n=0) force stopSineTest so each pitch change is a clean new start.
 * Energy: coin blip → rising arpeggio → power-up climb → short fanfare.
 */

/** Original 8-bit arcade jingle (~1.6 s): punchy mid-high phrases, not a long melody. */
static const AfMelodyNote kArcadeMelody[] PROGMEM = {
    // Coin blip
    {0x5C, 50}, {0x00, 30}, {0x6C, 65}, {0x00, 80},
    // Fast rising arpeggio
    {0x38, 40}, {0x00, 22}, {0x44, 40}, {0x00, 22}, {0x50, 40}, {0x00, 22},
    {0x5C, 40}, {0x00, 22}, {0x68, 75}, {0x00, 90},
    // Power-up climb
    {0x40, 35}, {0x00, 18}, {0x48, 35}, {0x00, 18}, {0x50, 35}, {0x00, 18},
    {0x58, 35}, {0x00, 18}, {0x60, 35}, {0x00, 18}, {0x6C, 85}, {0x00, 95},
    // Fanfare hits
    {0x54, 60}, {0x00, 35}, {0x54, 60}, {0x00, 35}, {0x5C, 45}, {0x00, 20},
    {0x64, 45}, {0x00, 20}, {0x6C, 110},
};

/**
 * @brief Play a PROGMEM note table once, then wait the configured interval.
 */
static void pollAlarmFallbackMelody(uint32_t now, const AfMelodyNote *table,
                                    uint8_t count) {
  if (alarmFallbackPhase == 0) {
    if (alarmFallbackStep >= count) {
      alarmFallbackSilence();
      alarmFallbackPhase = 1;
      alarmFallbackStep = 0;
      alarmFallbackNextMs = now + alarmFallbackGapMs();
      return;
    }
    const uint8_t n = pgm_read_byte(&table[alarmFallbackStep].n);
    // Packed struct: read duration as two bytes to avoid unaligned word warning.
    const uint8_t *msPtr = (const uint8_t *)&table[alarmFallbackStep].ms;
    const uint16_t ms =
        (uint16_t)pgm_read_byte(msPtr) | ((uint16_t)pgm_read_byte(msPtr + 1) << 8);
    if (n == 0) {
      alarmFallbackSilence();
    } else {
      alarmFallbackSetSine(n);
    }
    alarmFallbackStep++;
    alarmFallbackNextMs = now + (ms > 0 ? ms : 1);
  } else {
    alarmFallbackPhase = 0;
    alarmFallbackStep = 0;
    alarmFallbackNextMs = now;
  }
}

static void pollAlarmFallbackArcade(uint32_t now) {
  pollAlarmFallbackMelody(now, kArcadeMelody,
                          (uint8_t)(sizeof(kArcadeMelody) / sizeof(kArcadeMelody[0])));
}

/**
 * @brief Drive wake-tone timing; call from the stream task.
 */
void pollAlarmFallback(void) {
  if (!alarmFallbackPlaying || !vs1053player || !vs1053Available) {
    return;
  }

  const uint32_t now = millis();
  if ((int32_t)(now - alarmFallbackNextMs) < 0) {
    return;
  }

  if (!claimSPI("alrmfb_poll")) {
    return;
  }

  switch (alarmFallbackKind) {
  case AF_CHIRP:
    pollAlarmFallbackChirp(now);
    break;
  case AF_BIRDS:
    pollAlarmFallbackBirds(now);
    break;
  case AF_WATER:
    pollAlarmFallbackWater(now);
    break;
  case AF_CASCADE:
    pollAlarmFallbackCascade(now);
    break;
  case AF_ARCADE:
    pollAlarmFallbackArcade(now);
    break;
  default:
    pollAlarmFallbackBeep(now);
    break;
  }

  releaseSPI();
}

/**
 * @brief End the alarm session and any fallback tone (scheduled stop or cancel).
 */
void endAlarmSession(void) {
  stopAlarmFallback();
  alarmSessionActive = false;
}

#endif
