#ifndef GLOBALS_TYPES_H
#define GLOBALS_TYPES_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiMulti.h>
#include <FS.h>
#include <SD.h>
#include <vector>
#include <esp_partition.h>
#include "config/firmware_config.h"

enum alarmStatus {
  ALRM_NONE,
  ALRM_START,
  ALRM_STOP
}; // NONE=Nothing; START=last change was a start; STOP=last change was a stop

//**************************************************************************************************
// Several structs.                                                                                *
//**************************************************************************************************
struct scrseg_struct // For screen segments
{
  bool update_req; // Request update of screen
  uint16_t color;  // Textcolor
  uint16_t y;      // Begin of segment row
  uint16_t height; // Height of segment
  String str;      // String to be displayed
};

enum qdata_type { QDATA, QSTARTSONG, QSTOPSONG }; // datatyp in qdata_struct
struct qdata_struct {
  int datatyp;                                 // Identifier
  __attribute__((aligned(4))) uint8_t buf[32]; // Buffer for chunk
};

struct ini_struct {
  uint8_t reqvol;          // Requested volume
  uint8_t rtone[4];        // Requested bass/treble settings
  int16_t newpreset;       // Requested preset
  String clk_server;       // Server to be used for time of day clock
  int8_t clk_offset;       // Offset in hours with respect to UTC
  int8_t clk_dst;          // Number of hours shift during DST
  int8_t ir_pin;           // GPIO connected to output of IR decoder
  int8_t enc_clk_pin;      // GPIO connected to CLK of rotary encoder
  int8_t enc_dt_pin;       // GPIO connected to DT of rotary encoder
  int8_t enc_sw_pin;       // GPIO connected to SW of rotary encoder
  int8_t tft_cs_pin;       // GPIO connected to CS of TFT screen
  int8_t tft_dc_pin;       // GPIO connected to D/C or A0 of TFT screen
  int8_t tft_scl_pin;      // GPIO connected to SCL of i2c TFT screen
  int8_t tft_sda_pin;      // GPIO connected to SDA of I2C TFT screen
  int8_t tft_bl_pin;       // GPIO to activate BL of display
  int8_t tft_blx_pin;      // GPIO to activate BL of display (inversed logic)
  int8_t sd_cs_pin;        // GPIO connected to CS of SD card
  int8_t vs_cs_pin;        // GPIO connected to CS of VS1053
  int8_t vs_dcs_pin;       // GPIO connected to DCS of VS1053
  int8_t vs_dreq_pin;      // GPIO connected to DREQ of VS1053
  int8_t vs_shutdown_pin;  // GPIO to shut down the amplifier
  int8_t vs_shutdownx_pin; // GPIO to shut down the amplifier (inversed logic)
  int8_t spi_sck_pin;      // GPIO connected to SPI SCK pin
  int8_t spi_miso_pin;     // GPIO connected to SPI MISO pin
  int8_t spi_mosi_pin;     // GPIO connected to SPI MOSI pin
  uint16_t bat0;           // ADC value for 0 percent battery charge
  uint16_t bat100;         // ADC value for 100 percent battery charge
  String alrm_config;      // Alarm schedules as extensible JSON object
};

struct WifiInfo_t // For list with WiFi info
{
  uint8_t inx;      // Index as in "wifi_00"
  char *ssid;       // SSID for an entry
  char *passphrase; // Passphrase for an entry
};

struct nvs_entry {
  uint8_t Ns;    // Namespace ID
  uint8_t Type;  // Type of value
  uint8_t Span;  // Number of entries used for this item
  uint8_t Rvs;   // Reserved, should be 0xFF
  uint32_t CRC;  // CRC
  char Key[16];  // Key in Ascii
  uint64_t Data; // Data in entry
};

struct nvs_page // For nvs entries
{               // 1 page is 4096 bytes
  uint32_t State;
  uint32_t Seqnr;
  uint32_t Unused[5];
  uint32_t CRC;
  uint8_t Bitmap[32];
  nvs_entry Entry[126];
};

struct keyname_t // For keys in NVS
{
  char Key[16]; // Max length is 15 plus delimeter
};

//**************************************************************************************************
// Global data section.                                                                            *
//**************************************************************************************************
// There is a block ini-data that contains some configuration.  Configuration data is              *
// saved in the preferences by the web UI. On restart the new data will
// de read from these preferences.                                                                 *
// Items in ini_block can be changed by commands from webserver/Serial.                            *
//**************************************************************************************************
enum display_t {
  T_UNDEFINED,
  T_BLUETFT,
  T_OLED, // Various types of display
  T_DUMMYTFT,
  T_LCD1602I2C,
  T_ILI9341,
  T_NEXTION
};

enum datamode_t {
  INIT = 1,
  HEADER = 2,
  DATA = 4, // State for datastream
  METADATA = 8,
  PLAYLISTINIT = 16,
  PLAYLISTHEADER = 32,
  PLAYLISTDATA = 64,
  STOPREQD = 128,
  STOPPED = 256
};

enum enc_menu_t { VOLUME, PRESET, TRACK };

struct progpin_struct {
  int8_t gpio;
  bool reserved;
  bool avail;
  String command;
  bool cur;
};

struct touchpin_struct {
  int8_t gpio;
  bool reserved;
  bool avail;
  String command;
  bool cur;
  int16_t count;
};

#endif
