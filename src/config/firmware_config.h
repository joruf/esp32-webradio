#ifndef FIRMWARE_CONFIG_H
#define FIRMWARE_CONFIG_H

#define FIRMWARE "v2.75 2026-10-10"
#define WEBUI_CACHE_MAX_AGE 60
#define UPDATEHOST "www.loresoft.de"
#define BINFILE "/downloads/Esp32_radio.ino.esp32.bin"
#define TFTFILE "/Arduino/ESP32-Radio.tft"

/* Display driver: set by PlatformIO build flags when needed. Default = TFT radio. */
#if !defined(BLUETFT) && !defined(ILI9341) && !defined(OLED) && !defined(DUMMYTFT) && \
    !defined(LCD1602I2C) && !defined(NEXTION) && !defined(ST7701_28B)
#define BLUETFT
#endif

/* Shared modern dashboard path (BlueTFT 160x128 and ST7701 480x640). */
#if defined(BLUETFT) || defined(ST7701_28B)
#define RADIO_DASHBOARD_TFT 1
#endif

/* Default hardware profile id for this binary (overridable via -DHW_PROFILE_DEFAULT=...). */
#ifndef HW_PROFILE_DEFAULT
#define HW_PROFILE_DEFAULT "wroom_tft"
#endif

#define QSIZ 800
#define DEBUG_BUFFER_SIZE 150
#define NAME "radio"
/* NVS namespace must stay stable across renames of NAME (mDNS/hostname).
 * Changing it makes the device look "factory reset" while old keys still
 * occupy flash and starve new writes (alarm save fails, etc.). */
#define NVS_NAMESPACE "ESP32Webradio"
#define MAXPRESETS 200
#define NVSBUFSIZE 150
#define TIMEPOS -52
#define SDSPEED 1000000
#define METASIZ 1024
#define MAXKEYS 200
#define BACKLIGHT_TIME 600
#define MAX_DISPLAY_NETWORKS 3
#define WIFI_RECONNECT_INTERVAL 10000

/** Factory / first-boot / missing-NVS default volume (0..100). */
#define RADIO_DEFAULT_VOLUME 80

#endif
