#ifndef ST7701_USB_VS1053_H
#define ST7701_USB_VS1053_H

/**
 * @file st7701_usb_vs1053.h
 * @brief Free ESP32-S3 USB D-/D+ pads (GPIO19/20) for VS1053 MISO/DREQ.
 *
 * Native USB-Serial-JTAG owns those pads at boot. After WiFi is up, OTA remains
 * available; USB-CDC serial will stop once pads are released.
 */

#if defined(ST7701_28B) && defined(CONFIG_IDF_TARGET_ESP32S3)

#include <Arduino.h>
#include <driver/gpio.h>
#include "hal/usb_serial_jtag_ll.h"
#include "globals/app_prototypes.h"

inline void st7701ReleaseUsbPadsForVs1053(void) {
  // Disconnect USB FSLS PHY from GPIO19/20 so VS1053 can drive MISO/DREQ.
  usb_serial_jtag_ll_phy_enable_pad(false);
  gpio_reset_pin(GPIO_NUM_19);
  gpio_reset_pin(GPIO_NUM_20);
  pinMode(19, INPUT);
  pinMode(20, INPUT);
  dbgprint("BOOT: USB pads released (GPIO19/20 free for VS1053 MISO/DREQ)");
}

#else

inline void st7701ReleaseUsbPadsForVs1053(void) {}

#endif

#endif
