/**
 * @file encoder_switch.h
 * @brief Polled rotary-encoder push button with debounce and single/double-click
 *        detection (replaces ISR-based switch handling).
 */

#ifndef ENCODER_SWITCH_H
#define ENCODER_SWITCH_H

#include <SingleButtonHandler.h>

#define ENC_DOUBLE_CLICK_WINDOW_MS 350

static SingleButtonHandler *encoderSwitch = nullptr;
static uint8_t encPendingClicks = 0;
static unsigned long encLastReleaseMs = 0;
static bool encWaitingForMultiClick = false;

/**
 * @brief Reset pending single/double-click detection state.
 */
static void encResetPendingClicks() {
  encPendingClicks = 0;
  encWaitingForMultiClick = false;
}

/**
 * @brief Handle raw button events from SingleButtonHandler.
 *
 * @param type Button event type
 */
static void encoderSwitchCallback(ClickType type) {
  switch (type) {
    case ClickType::SHORT_CLICK_UP:
      encPendingClicks++;
      encLastReleaseMs = millis();
      encWaitingForMultiClick = true;
      enc_inactivity = 0;
      break;

    case ClickType::LONG_CLICK_DOWN:
      encResetPendingClicks();
      longclick = true;
      enc_inactivity = 0;
      break;

    case ClickType::SHORT_CLICK_DOWN:
    case ClickType::LONG_CLICK_UP:
    case ClickType::DURING_PRESS:
    default:
      break;
  }
}

/**
 * @brief Finalize single or double click after the inter-click timeout.
 */
static void encFinalizePendingClicks() {
  if (!encWaitingForMultiClick || encPendingClicks == 0) {
    return;
  }

  if ((millis() - encLastReleaseMs) < ENC_DOUBLE_CLICK_WINDOW_MS) {
    return;
  }

  if (encPendingClicks >= 2) {
    doubleclick = true;
  } else {
    singleclick = true;
  }

  encResetPendingClicks();
}

/**
 * @brief Initialize encoder push-button handling (no interrupt on SW pin).
 *
 * @param pin GPIO pin connected to encoder SW (active low)
 */
static void initEncoderSwitch(int8_t pin) {
  if (pin < 0) {
    return;
  }

  if (encoderSwitch != nullptr) {
    delete encoderSwitch;
    encoderSwitch = nullptr;
  }

  encResetPendingClicks();

  encoderSwitch = new SingleButtonHandler((uint8_t)pin, true, true);
  encoderSwitch->setDebounceTime(50);
  encoderSwitch->setClickThreshold(40);
  encoderSwitch->setLongClickThreshold(1000);
  encoderSwitch->addClickHandler(encoderSwitchCallback);
}

/**
 * @brief Poll encoder switch state; call every main loop iteration.
 */
static void updateEncoderSwitch() {
  if (encoderSwitch == nullptr) {
    return;
  }

  encoderSwitch->update();
  encFinalizePendingClicks();
}

/**
 * @brief Release encoder switch handler resources.
 */
static void deinitEncoderSwitch() {
  if (encoderSwitch != nullptr) {
    delete encoderSwitch;
    encoderSwitch = nullptr;
  }
  encResetPendingClicks();
}

#endif
