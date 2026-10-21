#ifndef ST7701_BOOT_WDT_H
#define ST7701_BOOT_WDT_H

#if defined(ST7701_28B)
#include <esp_task_wdt.h>

/** Feed the task WDT during long ST7701 / WiFi / VS1053 boot steps. */
inline void st7701FeedTaskWdt(void) {
  esp_task_wdt_reset();
}
#else
inline void st7701FeedTaskWdt(void) {}
#endif

#endif
