// filename: ESP32_Webradio.ino
//  Board > esp32 > ESP32 Dev Module
//  Sollte der Upload abbrechen mit "Fehlgeschlagenes Hochladen: Hochladefehler: exit status 2", den USB-Stecker
//  ausstecken, auf Hochladen klicken etwas warten bis "Kompiliere Scatch" bei 3/4 steht und dann USB wieder einstecken
//  während des ende der Kompilierungsphase BOARD-VERWALTUNG: esp32 (Espressif Systems) v3.3.0 Space overlaod>100% -->
//  Tools > Partition Scheme > Huge App (3MB no OTA)

// Upload Sketch: Port 7 on Samsung I right bottom. Use AVRISP mkII, Click Sketch Hochladen,

// ESP32 Infos at: http://esp32-server.de/

// ESP32dev Signal  Wired to LCD        Wired to VS1053      SDCARD   Wired to the rest
// -------- ------  --------------      -------------------  ------   ---------------
// GPIO32           -                   pin 1 XDCS            -       -
// GPIO5            -                   pin 2 XCS             -       -
// GPIO4            -                   pin 4 DREQ            -       -
// GPIO2            pin 3 D/C or A0     -                     -       -
// GPIO22           -                   -                     CS      -
// GPIO16   RXD2    -                   -                     -       TX of NEXTION (if in use)
// GPIO17   TXD2    -                   -                     -       RX of NEXTION (if in use)
// GPIO18   SCK     pin 5 CLK or SCL    pin 5 SCK             CLK     -
// GPIO19   MISO    -                   pin 7 MISO            MISO    -
// GPIO23   MOSI    pin 4 DIN or SDA    pin 6 MOSI            MOSI    -
// GPIO15           pin 2 CS            -                     -       -
// GPI03    RXD0    -                   -                     -       Reserved serial input
// GPIO1    TXD0    -                   -                     -       Reserved serial output
// GPIO33   Backl.  -                   LED+                  -       -
// GPIO34   -       -                   -                     -       Optional pull-up resistor
// GPIO35   -       -                   -                     -       Infrared receiver VS1838B
// GPIO25   -       -                   -                     -       Rotary encoder CLK
// GPIO26   -       -                   -                     -       Rotary encoder DT
// GPIO27   -       -                   -                     -       Rotary encoder SW
// -------  ------  ---------------     -------------------  ------   ----------------
// GND      -       pin 8 GND           pin 8 GND                     Power supply GND
// VCC 5 V  -       pin 7 BL            -                             Power supply
// VCC 5 V  -       pin 6 VCC           pin 9 5V                      Power supply
// EN       -       pin 1 RST           pin 3 XRST                    -

// Define the version number, also used for webserver as Last-Modified header and to
// check version for update.  The format must be exactly as specified by the HTTP standard!
#define FIRMWARE "v2.13 2026-06-17"

// Web UI cache duration in seconds for static files (HTML, CSS, JS).
// 0 = caching disabled. Values >= 1 enable Cache-Control: max-age=<seconds>.
#define WEBUI_CACHE_MAX_AGE 60
/*
Firmware Changes:
2.00  2025-03-06  Reload time changes on problems with the data connection.
2.01  2025-03-27  Responsive Design meta tag added to adapt the content on differenz screen sizes.
2.02  2025-05-07  Bugfix HTTP server for root directory "/". HTML tags and stylesheets adapted for menu.
2.03  2026-01-15  Song title API for webinterface implemented. Now we can see icystreamtitle and icyname.
                  Sleep-Time implemented to stop mp3 stream after a defined time.
                  Const VERSION renamed to FIRMWARE.
2.04  2026-01-16  Update ESP32 Espressif-Systems v2.0.17 to 3.3.0.
2.05  2026-01-16  Rotary encoder click and doubleclick logic implemented click=radio on/off, doubleclick=select
                  radio station.
2.06  2026-01-17  Bugfix alarm start/stop not working after ESP32 Espressif-Systems update to 3.3.0.
2.07  2026-01-21  MQTT removed, mute button removed from index.html, html button size changed. Hardcoded GPIO pins for
                  the rotary encoder, IR receiver, SD card, TFT, and VS1053 fixed within the code. Webinterface layout
                  adapted.
2.08 2026-01-22   writeDefaultConfig.h implemented to write the default props into the NVS on problems. SD-Card
                  disabled. Display NetworkList listNetworks() into the display.
2.09  2026-01-22  IR remote control recognition removed. defaultprefs.h structured
2.10  2026-06-06  Code switch from Arduino IDE to
                  VSCode PlatformIO Bugfixes vs1053 semaphores Time format expanded with date
2.11  2026-06-15  Web config as JSON/NVS with form UI. Scheduler page (sleep timer, alarms). Preset reorder.
                  Bugfixes playlist redirect, EEPROM/config load, alarm save.
2.12  2026-06-16  Configuration auto-saves immediately on field change (select change sends setting at once).
                  UTC offset and DST as select boxes (0 or 1 only); other numeric options use select boxes.
                  Sound settings (volume, equalizer) moved from Configuration to Sound Settings page.
                  HTTP POST config save fixed (Content-Length, full body read, save error feedback).
                  Scheduler: removing an alarm now persists to NVS.
2.13  2026-06-17  Alarm scheduler fixed: weekday check, reliable start/stop, per-second task handling.
                  Boot preset select lists only configured presets. Scheduler alarm times as hh:mm masked inputs
                  (two-digit entry, e.g. 20:00 and 23:59). Save/remove alarm feedback in command log.
                  Clock/NTP: descriptive labels and tooltips; UTC offset UTC-11..UTC+14, DST Off/On.
                  Clock settings apply immediately (applyClockConfiguration); command log shows updated time.
                  WEBUI_CACHE_MAX_AGE for static UI caching (60s). HTTP server stability improvements.
                  Soft volume fade in/out on radio play start and stop (2s).
*/

// ESP32-Radio can be updated (OTA) to the latest version from a remote server.
// The download uses the following server and files:
#define UPDATEHOST "www.loresoft.de"                   // Host for software updates
#define BINFILE "/downloads/Esp32_radio.ino.esp32.bin" // Binary file name for update software
#define TFTFILE "/Arduino/ESP32-Radio.tft"             // Binary file name for update NEXTION image

// Define (just one) type of display.  See documentation.
#define BLUETFT // Works also for RED TFT 128x160
// #define OLED                         // 64x128 I2C OLED
// #define DUMMYTFT                     // Dummy display
// #define LCD1602I2C                   // LCD 1602 display with I2C backpack
// #define ILI9341                      // ILI9341 240*320
// #define NEXTION                      // Nextion display. Uses UART 2 (pin 16 and 17)

#include <Arduino.h>
#include <stdio.h>  // Für Standard-Eingabe/Ausgabe-Operationen
#include <string.h> // Für strlen, strstr etc.

#include "nvs.h"
#include <PubSubClient.h>
#include <WiFiMulti.h> // Datei > Einstellungen > Zusätzliche Boardverwalter-URLs > https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json > Board-Verwaltung > install: esp32 (Espressif Systems)
// #include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoOTA.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include <esp_partition.h>
#include <driver/adc.h>
#include <Update.h>
#include <base64.h>

#define QSIZ 400              // Number of entries in the queue
#define DEBUG_BUFFER_SIZE 150 // Debug buffer size
#define NVSBUFSIZE 150

// Access point name if connection to WiFi network fails.  Also the hostname for WiFi and OTA.
// Note that the password of an AP must be at least as long as 8 characters.
// Also used for other naming.
#define NAME "ESP32Webradio"

#define MAXPRESETS 200         // Max number of presets in preferences
#define NVSBUFSIZE 150         // Adjust size of buffer to the longest expected string for nvsGetValue
#define TIMEPOS -52            // Position (column) of time in topline relative to end
#define SDSPEED 1000000        // SPI speed for SD card
#define METASIZ 1024           // Size of metaline buffer
#define MAXKEYS 200            // Max. number of NVS keys in table
#define BACKLIGHT_TIME 600     // Time-out [sec] for blanking TFT display (BL pin)
#define MAX_DISPLAY_NETWORKS 3 // Max number of displayed networks on the display
#define otaclient mp3client    // OTA uses mp3client for connection to host

// 10sec adaption
#define WIFI_RECONNECT_INTERVAL 10000 // 10 Sekunden in Millisekunden
static uint32_t lastReconnectAttempt = 0;

//**************************************************************************************************
// Forward declaration and prototypes of various functions.                                        *
//**************************************************************************************************
void showTimeOnTFT(const char *str, uint16_t color = 0xFFFF);
void showStreamDataOnTFT(const char *ml, bool full = false);
void handleNextDataByteFromServer(uint8_t b);
void handleWebpage(const String &pagename);
void handleCmd();
char *dbgprint(const char *format, ...);
const char *analyzeCmd(const char *str);
const char *analyzeCmd(const char *par, const char *val);
void chomp(String &str);
String httpheader(String contentstype);
void recoverHttpServerState();
bool nvsKeyExists(const char *key);
void mp3loop();
void tftlog(const char *str);
void playStreamFromQueue(void *parameter); // Task to play the stream
void spftask(void *parameter);             // Task for special functions
void getCurrentTime();
void applyClockConfiguration(bool restartNtp);
void reservepin(int8_t rpinnr);

bool isSoundPlaying();
void stopStream();
bool playStream(int16_t _preset); // true=sending play info succeeded; false= unable to send playing info
void playSound(bool _enable);
void stop_mp3client();
void IRAM_ATTR enableBacklight(bool enable);
void handleDisplayData();
String nvsGetHostFromPref(int16_t preset);
String nvsGetHostFromPref();
void showTextOnTFT(uint16_t inx, const char *str);
void showTextOnTFT(uint16_t inx, String &str);

enum alarmStatus {
  ALRM_NONE,
  ALRM_START,
  ALRM_STOP
}; // NONE=Nothing; START=last change was a start; STOP=last change was a stop
alarmStatus radioAlarmStatusChanged =
    ALRM_NONE; // Indicates if the status was changed right now, to update the display one minute later

bool vs1053Available = false;
const char *msgPlayerStopped = "Player stopped!\nclick\nto select new station"; // 26 char per line
const char *msgTurnToSelectStation = "Turn to select station\nPress to confirm";
const char *msgRadioAlarmOn = "Radio Alarm\nhas turned the music ON";
const char *msgRadioAlarmOff = "Radio Alarm\nhas turned the music OFF";

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
// saved in the preferences by the webinterface.  On restart the new data will                     *
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

// Global variables
int DEBUG = 1;                     // Debug on/off
int numSsid;                       // Number of available WiFi networks
WiFiMulti wifiMulti;               // Possible WiFi networks
ini_struct ini_block;              // Holds configurable data
WiFiServer cmdserver(80);          // Instance of embedded webserver, port 80
WiFiClient mp3client;              // An instance of the mp3 client, also used for OTA
WiFiClient cmdclient;              // An instance of the client for commands
HardwareSerial *nxtserial = NULL;  // Serial port for NEXTION (if defined)
TaskHandle_t maintask;             // Taskhandle for main task
TaskHandle_t xplayStreamFromQueue; // Task handle for playStreamFromQueue
TaskHandle_t xspftask;             // Task handle for special functions
SemaphoreHandle_t SPIsem = NULL;   // For exclusive SPI usage
hw_timer_t *timer = NULL;          // For timer
char timetxt[40];                  // Enough chars for date and time. Converted timeinfo
char cmd[130];                     // Command from Serial
uint8_t tmpbuff[6000];             // Input buffer for mp3 or data stream
QueueHandle_t dataqueue;           // Queue for mp3 datastream
QueueHandle_t spfqueue;            // Queue for special functions
qdata_struct outchunk;             // Data to queue
qdata_struct inchunk;              // Data from queue
uint8_t *outqp = outchunk.buf;     // Pointer to buffer in outchunk
uint32_t totalcount = 0;           // Counter mp3 data
datamode_t datamode;               // State of datastream
int metacount;                     // Number of bytes in metadata
int datacount;                     // Counter databytes before metadata
char metalinebf[METASIZ + 1];      // Buffer for metaline/ID3 tags
int16_t metalinebfx;               // Index for metalinebf

String icystreamtitle; // Streamtitle from metadata --> Song title
String icyname;        // Icecast station name --> SWR3,...

String ipaddress;                      // Own IP-address
int bitrate;                           // Bitrate in kb/sec
int mbitrate;                          // Measured bitrate
int metaint = 0;                       // Number of databytes between metadata
int16_t currentpreset = -1;            // Preset station playing
String host;                           // The URL to connect to or file to play
String playlist;                       // The URL of the specified playlist
bool hostreq = false;                  // Request for new host
bool reqtone = false;                  // New tone setting requested
bool muteflag = false;                 // Mute output
bool resetreq = false;                 // Request to reset the ESP32
bool updatereq = false;                // Request to update software from remote host
bool NetworkFound = false;             // True if WiFi network connected
String networks;                       // Found networks in the surrounding
int16_t playlist_num = 0;              // Nonzero for selection from playlist
File mp3file;                          // File containing mp3 on SD card
uint32_t mp3filelength;                // File length
bool localfile = false;                // Play from local mp3-file or not
bool chunked = false;                  // Station provides chunked transfer
int chunkcount = 0;                    // Counter for chunked transfer
String http_getcmd;                    // Contents of last GET command
String http_rqfile;                    // Requested file
bool http_response_flag = false;       // Response required
uint32_t http_post_content_length = 0; // POST body size from Content-Length header
uint16_t ir_value = 0;                 // IR code
uint32_t ir_0 = 550;                   // Average duration of an IR short pulse
uint32_t ir_1 = 1650;                  // Average duration of an IR long pulse
struct tm timeinfo;                    // Will be filled by NTP server
bool time_req = false;                 // Set time requested
bool SD_okay = false;                  // True if SD card in place and readable
String SD_nodelist;                    // Nodes of mp3-files on SD
int SD_nodecount = 0;                  // Number of nodes in SD_nodelist
String SD_currentnode = "";            // Node ID of song playing ("0" if random)
uint16_t adcval;                       // ADC value (battery voltage)
uint32_t clength;                      // Content length found in http header
uint32_t max_mp3loop_time = 0;         // To check max handling time in mp3loop (msec)
int16_t scanios;                       // TEST*TEST*TEST
int16_t scaniocount;                   // TEST*TEST*TEST
uint16_t bltimer = 0;                  // Backlight time-out counter
display_t displaytype = T_UNDEFINED;   // Display type
std::vector<WifiInfo_t> wifilist;      // List with wifi_xx info
// nvs stuff
nvs_page nvsbuf;                 // Space for 1 page of NVS info
const esp_partition_t *nvs;      // Pointer to partition struct
esp_err_t nvserr;                // Error code from nvs functions
uint32_t nvshandle = 0;          // Handle for nvs access
uint8_t namespace_ID;            // Namespace ID found
char nvskeys[MAXKEYS][16];       // Space for NVS keys
std::vector<keyname_t> keynames; // Keynames in NVS
// Rotary encoder stuff
#define sv DRAM_ATTR static volatile
sv int16_t rotationcount = 0;              // Current position of rotary switch
sv uint16_t enc_inactivity = 0;            // Time inactive
sv bool singleclick = false;               // True if single click detected
sv bool doubleclick = false;               // True if double click detected
sv bool longclick = false;                 // True if longclick detected
enum enc_menu_t { VOLUME, PRESET, TRACK }; // State for rotary encoder menu
enc_menu_t enc_menu_mode = VOLUME;         // Default is VOLUME mode

struct progpin_struct // For programmable input pins
{
  int8_t gpio;    // Pin number
  bool reserved;  // Reserved for connected devices
  bool avail;     // Pin is available for a command
  String command; // Command to execute when activated
  // Example: "uppreset=1"
  bool cur; // Current state, true = HIGH, false = LOW
};

progpin_struct progpin[] = // Input pins and programmed function
    {
        {0, false, false, "", false},
        //{  1, true,  false,  "", false },                    // Reserved for TX Serial output
        {2, false, false, "", false},
        //{  3, true,  false,  "", false },                    // Reserved for RX Serial input
        {4, false, false, "", false},
        {5, false, false, "", false},
        //{  6, true,  false,  "", false },                    // Reserved for FLASH SCK
        //{  7, true,  false,  "", false },                    // Reserved for FLASH D0
        //{  8, true,  false,  "", false },                    // Reserved for FLASH D1
        //{  9, true,  false,  "", false },                    // Reserved for FLASH D2
        //{ 10, true,  false,  "", false },                    // Reserved for FLASH D3
        //{ 11, true,  false,  "", false },                    // Reserved for FLASH CMD
        {12, false, false, "", false},
        {13, false, false, "", false},
        {14, false, false, "", false},
        {15, false, false, "", false},
        {16, false, false, "", false}, // May be UART 2 RX for Nextion
        {17, false, false, "", false}, // May be UART 2 TX for Nextion
        {18, false, false, "", false}, // Default for SPI CLK
        {19, false, false, "", false}, // Default for SPI MISO
        //{ 20, true,  false,  "", false },                    // Not exposed on DEV board
        {21, false, false, "", false}, // Also Wire SDA
        {22, false, false, "", false}, // Also Wire SCL
        {23, false, false, "", false}, // Default for SPI MOSI
        //{ 24, true,  false,  "", false },                    // Not exposed on DEV board
        {25, false, false, "", false},
        {26, false, false, "", false},
        {27, false, false, "", false},
        //{ 28, true,  false,  "", false },                    // Not exposed on DEV board
        //{ 29, true,  false,  "", false },                    // Not exposed on DEV board
        //{ 30, true,  false,  "", false },                    // Not exposed on DEV board
        //{ 31, true,  false,  "", false },                    // Not exposed on DEV board
        {32, false, false, "", false},
        {33, false, false, "", false},
        {34, false, false, "", false}, // Note, no internal pull-up
        {35, false, false, "", false}, // Note, no internal pull-up
        //{ 36, true,  false,  "", false },                    // Reserved for ADC battery level
        {39, false, false, "", false}, // Note, no internal pull-up
        {-1, false, false, "", false}  // End of list
};

struct touchpin_struct // For programmable input pins
{
  int8_t gpio;    // Pin number GPIO
  bool reserved;  // Reserved for connected devices
  bool avail;     // Pin is available for a command
  String command; // Command to execute when activated
  // Example: "uppreset=1"
  bool cur;      // Current state, true = HIGH, false = LOW
  int16_t count; // Counter number of times low level
};
touchpin_struct touchpin[] = // Touch pins and programmed function
    {
        {4, false, false, "", false, 0},  // TOUCH0
        {0, true, false, "", false, 0},   // TOUCH1, reserved for BOOT button
        {2, false, false, "", false, 0},  // TOUCH2
        {15, false, false, "", false, 0}, // TOUCH3
        {13, false, false, "", false, 0}, // TOUCH4
        {12, false, false, "", false, 0}, // TOUCH5
        {14, false, false, "", false, 0}, // TOUCH6
        {27, false, false, "", false, 0}, // TOUCH7
        {33, false, false, "", false, 0}, // TOUCH8
        {32, false, false, "", false, 0}, // TOUCH9
        {-1, false, false, "", false, 0}  // End of list
                                          // End of table
};

//**************************************************************************************************
//     Webinterface.                                                                               *
//**************************************************************************************************
#include "webinterface/index.html.h"
#include "webinterface/mp3play.html.h"
#include "webinterface/sound.html.h"
#include "webinterface/config.html.h"
#include "webinterface/scheduler.html.h"
#include "webinterface/firmware.html.h"
#include "webinterface/about.html.h"
#include "webinterface/index.css.h"
#include "webinterface/interface.js.h"
/*#include "webinterface/favicon.ico.h"*/
#include "defaultprefs.h"

//**************************************************************************************************
//     VS1053 stuff.                                                                               *
//**************************************************************************************************
#include "handler/vs1053.h"
VS1053 *vs1053player;

#include "handler/nvs.h"
#include "handler/string.h"
#include "handler/isr.h"

//**************************************************************************************************
// Display Handling.                                                                               *
//**************************************************************************************************
// Include software for the right display
#ifdef BLUETFT
#  include "display/BlueTFT.h" // For ILI9163C or ST7735S 128x160 display
#endif
#ifdef ILI9341
#  include "display/ILI9341.h" // For ILI9341 320x240 display
#endif
#ifdef OLED
#  include "display/SSD1306.h" // For OLED I2C SD1306 64x128 display
#endif
#ifdef LCD1602I2C
#  include "display/LCD1602.h" // For LCD 1602 display (I2C)
#endif
#ifdef DUMMYTFT
#  include "display/DummyTFT.h" // For Dummy display
#endif
#ifdef NEXTION
#  include "display/NEXTION.h" // For NEXTION display
#endif

//**************************************************************************************************
//                                      Handler Includes                                           *
//**************************************************************************************************
#include "handler/spi.h"
#include "handler/helper.h"
#include "handler/alarm_config.h"
#include "handler/radio_config.h"
#include "handler/sd.h"
#include "handler/upgrade.h"
#include "handler/volume.h"
#include "handler/alarm.h"
#include "handler/loop.h"
#include "handler/setup.h"
#include "handler/xml.h"
#include "handler/time.h"
#include "handler/http.h"
#include "handler/rotary_encoder.h"
#include "handler/encoder_switch.h"
#include "handler/interface.h"
#include "handler/display.h"
#include "handler/stream.h"
#include "handler/mp3.h"
#include "writeDefaultConfig.h"

// ═════════════════════════════════════════════════════════════════
// C H E C K   A N D   I N I T   C O N F I G
// ═════════════════════════════════════════════════════════════════
/**
 * @brief Count active NVS keys in the global key table.
 *
 * @return uint16_t Number of stored keys
 */
static uint16_t countNvsKeys() {
  uint16_t i = 0;

  while (nvskeys[i][0] != '\0' && i < MAXKEYS) {
    i++;
  }
  return i;
}

/**
 * @brief Validate WiFi credentials format SSID/password.
 *
 * @param wifi Raw NVS value
 * @return true if format is valid
 */
static bool isValidWifiEntry(const String &wifi) {
  int sep = wifi.indexOf('/');
  if (sep <= 0) {
    return false;
  }

  String ssid = wifi.substring(0, sep);
  String pass = wifi.substring(sep + 1);
  ssid.trim();
  pass.trim();
  return (ssid.length() > 0 && pass.length() > 0);
}

/**
 * @brief Check whether at least one radio preset exists in NVS.
 *
 * @return true if a preset entry was found
 */
static bool hasAnyPresetInNvs() {
  char key[12];
  uint16_t i;

  for (i = 0; i < MAXPRESETS; i++) {
    sprintf(key, "preset_%02d", i);
    if (nvsKeyExists(key) && nvsGetValue(key).length() > 0) {
      return true;
    }
  }

  return false;
}

/**
 * @brief Validate the current NVS configuration.
 *
 * @param reason Output text describing the validation failure
 * @return true if configuration is usable
 */
static bool validateNvsConfig(String &reason) {
  if (!nvsKeyExists("wifi_00")) {
    reason = "missing wifi_00";
    return false;
  }

  if (!isValidWifiEntry(nvsGetValue("wifi_00"))) {
    reason = "invalid wifi_00 format (expected SSID/password)";
    return false;
  }

  if (!nvsKeyExists("preset")) {
    reason = "missing active preset key";
    return false;
  }

  if (!hasAnyPresetInNvs()) {
    reason = "no radio presets found";
    return false;
  }

  if (!nvsKeyExists("volume")) {
    reason = "missing volume key";
    return false;
  }

  if (!nvsKeyExists("clk_server")) {
    reason = "missing time server key";
    return false;
  }

  if (nvsKeyExists("alrm_config")) {
    String alarmJson = nvsGetAlarmConfig();
    if (!isValidAlarmConfigJson(alarmJson)) {
      reason = "invalid alrm_config JSON";
      return false;
    }
  } else if (nvsKeyExists(RADIO_CONFIG_NVS_KEY)) {
    String radioJson = nvsGetRadioConfig();
    if (!isValidRadioConfigJson(radioJson)) {
      reason = "invalid radio_config JSON";
      return false;
    }
  }

  return true;
}

/**
 * @brief Validate NVS on boot and restore defaults if configuration is missing
 *        or corrupted.
 */
void checkAndInitConfig() {
  String reason;

  if (!nvsKeyExists("wifi_00")) {
    dbgprint("BOOT: No NVS configuration found (first start or reset)");
    restoreDefaultConfig();
    dbgprint("BOOT: Default configuration written, restarting...");
    delay(1000);
    ESP.restart();
    return;
  }

  if (!validateNvsConfig(reason)) {
    dbgprint("BOOT: Invalid NVS configuration detected: %s", reason.c_str());
    restoreDefaultConfig();
    dbgprint("BOOT: Default configuration restored, restarting...");
    delay(1000);
    ESP.restart();
    return;
  }

  dbgprint("BOOT: NVS configuration validation passed");
}

//**************************************************************************************************
//                                           S E T U P                                             *
//**************************************************************************************************
// Setup for the program.                                                                          *
//**************************************************************************************************
void setup() {
  int i;     // Loop control
  int pinnr; // Input pinnumber
  const char *p;
  byte mac[6];                  // WiFi mac address
  char tmpstr[20];              // For version and Mac address
  const char *partname = "nvs"; // Partition with NVS info
  esp_partition_iterator_t pi;  // Iterator for find
  const char *dtyp = "Display type is %s";
  const char *wvn = "Include file %s_html has the wrong version number! "
                    "Replace header file.";

  Serial.begin(115200); // For debug
  Serial.println();
  dbgprint("========================================");
  dbgprint("BOOT: ESP32 Webradio starting");
  dbgprint("BOOT: Firmware %s", FIRMWARE);
  dbgprint("BOOT: CPU core %d at %d MHz", xPortGetCoreID(), ESP.getCpuFreqMHz());
  dbgprint("BOOT: Free heap %d bytes", ESP.getFreeHeap());
  dbgprint("========================================");

  // ATTENTION NVS config problems (joruf)
  // Uncomment saveDefaultConfig() on access problems to re-write configuration props to NVS
  // saveDefaultConfig();

#if defined(BLUETFT) // Report display option
  dbgprint(dtyp, "BLUETFT");
#endif
#if defined(ILI9341) // Report display option
  dbgprint(dtyp, "ILI9341");
#endif
#if defined(OLED)
  dbgprint(dtyp, "OLED");
#endif
#if defined(DUMMYTFT)
  dbgprint(dtyp, "DUMMYTFT");
#endif
#if defined(LCD1602I2C)
  dbgprint(dtyp, "LCD1602");
#endif
#if defined(NEXTION)
  dbgprint(dtyp, "NEXTION");
#endif

  maintask = xTaskGetCurrentTaskHandle(); // My taskhandle
  SPIsem = xSemaphoreCreateMutex();
  ;                                                  // Semaphore for SPI bus
  pi = esp_partition_find(ESP_PARTITION_TYPE_DATA,   // Get partition iterator for
                          ESP_PARTITION_SUBTYPE_ANY, // the NVS partition
                          partname);
  if (pi) {
    nvs = esp_partition_get(pi);        // Get partition struct
    esp_partition_iterator_release(pi); // Release the iterator
    dbgprint("Partition %s found, %d bytes", partname, nvs->size);
  } else {
    dbgprint("Partition %s not found!", partname); // Very unlikely...
    while (true)
      ; // Impossible to continue
  }
  namespace_ID = FindNamespaceId(NAME); // Find ID of our namespace in NVS
  fillkeylist();                        // Fill keynames with all keys
  dbgprint("BOOT: NVS namespace '%s' ready, %d key(s) found", NAME, countNvsKeys());

  checkAndInitConfig();
  migrateToRadioConfig();

  memset(&ini_block, 0, sizeof(ini_block)); // Init ini_block
  ini_block.clk_server = "pool.ntp.org";    // Default server for NTP
  ini_block.clk_offset = 1;                 // Default Amsterdam time zone
  ini_block.clk_dst = 1;                    // DST is +1 hour
  ini_block.bat0 = 0;                       // Battery ADC levels not yet defined
  ini_block.bat100 = 0;
  ini_block.alrm_config = "";

  readIOprefs(); // Read pins used for SPI, TFT, VS1053, IR,
  // Rotary encoder
  for (i = 0; (pinnr = progpin[i].gpio) >= 0; i++) // Check programmable input pins
  {
    pinMode(pinnr, INPUT_PULLUP); // Input for control button
    delay(10);
    // Check if pull-up active
    if ((progpin[i].cur = digitalRead(pinnr)) == HIGH) {
      p = "HIGH";
    } else {
      p = "LOW, probably no PULL-UP"; // No Pull-up
    }
    dbgprint("GPIO%d is %s", pinnr, p);
  }

  readprogbuttons(); // Program the free input pins

  SPI.begin(ini_block.spi_sck_pin, ini_block.spi_miso_pin,
            ini_block.spi_mosi_pin); // Init VSPI bus with default or modified pins
  vs1053player = new VS1053(ini_block.vs_cs_pin, ini_block.vs_dcs_pin, ini_block.vs_dreq_pin, ini_block.vs_shutdown_pin,
                            ini_block.vs_shutdownx_pin); // Make instance of player
  if (ini_block.ir_pin >= 0) {
    dbgprint("Enable pin %d for IR", ini_block.ir_pin);
    pinMode(ini_block.ir_pin, INPUT);                  // Pin for IR receiver VS1838B
    attachInterrupt(ini_block.ir_pin, isr_IR, CHANGE); // Interrupts will be handle by isr_IR
  }

  if ((ini_block.tft_cs_pin >= 0) || (ini_block.tft_scl_pin >= 0)) { // Display configured?
    dbgprint("Start display");
    if (dsp_begin()) {         // Init display
      dsp_setRotation();       // Use landscape format
      dsp_erase();             // Clear screen
      dsp_setTextSize(1);      // Small character font
      dsp_setTextColor(WHITE); // Info in white
      dsp_setCursor(0, 0);     // Top of screen

      dsp_println("Loresoft, www.loresoft.de");

      p = dbgprint("Firmware %s", FIRMWARE);
      dsp_println(p);

      // Print some memory and sketch info
      p = dbgprint("%d CPU, %d MHz", xPortGetCoreID(), ESP.getCpuFreqMHz()); // Normally about 170 kB
      dsp_println(p);

      p = dbgprint("Free memory %d KByte", (ESP.getFreeHeap() / 1000)); // Normally about 170 kB
      dsp_println(p);

      dsp_println("");
      dsp_println("Booting Device...");

      dsp_update(); // Show on physical screen
    }
  }
  if (ini_block.tft_bl_pin >= 0) // Backlight for TFT control?
  {
    pinMode(ini_block.tft_bl_pin, OUTPUT); // enable output
  }
  if (ini_block.tft_blx_pin >= 0) // Backlight for TFT (inversed logic) control?
  {
    pinMode(ini_block.tft_blx_pin, OUTPUT); // enable output
  }
  enableBacklight(true); // Enable backlight (if configured)

  // SD Card mount disabled. We dont have one
  /*
  if(ini_block.sd_cs_pin >= 0)                       // SD configured?
  {
    mountSDCard();
  }
  */

  mk_lsan(); // Make a list of acceptable networks
             // in preferences.
  dbgprint("BOOT: %d WiFi credential(s) loaded from NVS", wifilist.size());
  WiFi.mode(WIFI_STA);      // This ESP is a station
  WiFi.persistent(false);   // Do not save SSID and password
  WiFi.disconnect();        // After restart router could still
  delay(100);               // keep old connection
  listNetworks();           // Find WiFi networks
  readprefs(false);         // Read preferences
  loadAlarmConfigFromNvs(); // Load or migrate alarm JSON configuration
  dbgprint("BOOT: Preferences loaded, active preset %d, volume %d", ini_block.newpreset, ini_block.reqvol);
  // tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, NAME);
  WiFi.setHostname(NAME);

  // Initialize VS1053 player
  vs1053player->begin();
  uint16_t modeReg = vs1053player->read_register(0x00); // 0x00 ist SCI_MODE

  if (modeReg == 0x0000 || modeReg == 0xFFFF) {
    dbgprint("VS1053 not properly installed! Register-Dump: 0x%04X", modeReg);
    vs1053Available = false;
  } else {
    dbgprint("VS1053 erfolgreich initialisiert. Mode-Register: 0x%04X", modeReg);
    vs1053Available = true;
  }

  delay(10);
  p = dbgprint("BOOT: Connecting to WiFi"); // Show progress
  tftlog(p);                                // On TFT too
  NetworkFound = connectwifi();             // Connect to WiFi network
  logNetworkBootStatus();
  dbgprint("BOOT: Starting HTTP command server");
  delay(500);
  cmdserver.begin(); // Start http server
  if (NetworkFound)  // OTA only if Wifi network found
  {
    dbgprint("BOOT: Internet access available, starting OTA and mDNS");
    ArduinoOTA.setHostname(NAME); // Set the hostname
    ArduinoOTA.onStart(otastart);
    ArduinoOTA.begin(); // Allow update over the air

    if (MDNS.begin(NAME)) { // Start MDNS transponder
      dbgprint("BOOT: mDNS responder started as %s.local", NAME);
    } else {
      dbgprint("BOOT: ERROR - mDNS responder setup failed");
    }
  } else {
    dbgprint("BOOT: Station mode unavailable, configuration AP or offline mode active");
    currentpreset = ini_block.newpreset; // No network: newpreset is -1, so means do not start radio
  }

  /*
  timer = timerBegin(0, 80, true);                   // User 1st timer with prescaler 80
  timerAttachInterrupt(timer, &timer100, true);      // Call timer100() on timer alarm
  timerAlarmWrite(timer, 100000, true);              // Alarm every 100 msec
  timerAlarmEnable(timer);                           // Enable the timer
  */
  timer = timerBegin(0, 80, true); // Timer 0 initialisieren, Teiler 80 (80 MHz / 80 = 1 MHz -> 1 Ticker pro
                                   // Mikrosekunde), true = hochzählen
  timerAttachInterrupt(
      timer, &timer100,
      true); // Interrupt-Funktion anhängen (v2-Standard verlangt das dritte Argument 'true' für Edge-Trigger)
  timerAlarmWrite(timer, 100000, true); // Alarm setzen: 100.000 Ticks = 100ms, true = Autoreload (wiederholen)
  timerAlarmEnable(timer);              // Den Timer aktivieren

  delay(1000); // Show IP for a while
  configTime(ini_block.clk_offset * 3600, ini_block.clk_dst * 3600,
             ini_block.clk_server.c_str()); // GMT offset, daylight offset in seconds
  timeinfo.tm_year = 0;                     // Set TOD to illegal

  // Init settings for rotary switch (if existing).
  // if(( ini_block.enc_clk_pin + ini_block.enc_dt_pin + ini_block.enc_sw_pin) <= 2 ) { // If rotary encoder prefs are
  // missed, probably there are no prefs saved. Write the prefs new
  //  saveDefaultConfig();
  //  dbgprint("All default prefs added, please restart the machine!");
  //}
  if ((ini_block.enc_clk_pin + ini_block.enc_dt_pin + ini_block.enc_sw_pin) > 2) {
    attachInterrupt(ini_block.enc_clk_pin, isr_enc_turn, CHANGE);
    attachInterrupt(ini_block.enc_dt_pin, isr_enc_turn, CHANGE);
    initEncoderSwitch(ini_block.enc_sw_pin);
    dbgprint("Rotary encoder is enabled");
  } else {
    dbgprint("Rotary encoder is disabled (%d/%d/%d)", ini_block.enc_clk_pin, ini_block.enc_dt_pin,
             ini_block.enc_sw_pin);
  }

  if (NetworkFound) {
    getCurrentTime(); // Sync time (write it in the register timetxt)
  }

  if (tft) {
    dsp_fillRect(0, 8, dsp_getwidth(), dsp_getheight() - 8, BLACK); // Clear most of the screen
  }

  outchunk.datatyp = QDATA; // This chunk dedicated to QDATA

  /*
  adc1_config_width(ADC_WIDTH_12Bit);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_0db);
  */
  adc1_config_width(ADC_WIDTH_BIT_12);                       // Correct Enum for deprecated function
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0); // New Enum Name

  dataqueue = xQueueCreate(QSIZ, sizeof(qdata_struct)); // Create queue for communication
  if (vs1053Available) {
    dbgprint("▶️ VS1053 available. Start Audio-Task...");
    xTaskCreatePinnedToCore(playStreamFromQueue,   // Task to play data in dataqueue.
                            "playStreamFromQueue", // name of task.
                            1600,                  // Stack size of task
                            NULL,                  // parameter of the task
                            2,                     // priority of the task
                            &xplayStreamFromQueue, // Task handle to keep track of created task
                            0);                    // Run on CPU 0
  } else {
    dbgprint("⚠️ VS1053 missing oder invalid! Audio-Task is disable, to avoid SPI-Blocking.");
  }

  xTaskCreate(spftask,    // Task to handle special functions.
              "Spftask",  // name of task.
              2048,       // Stack size of task
              NULL,       // parameter of the task
              1,          // priority of the task
              &xspftask); // Task handle to keep track of created task
}

//**************************************************************************************************
//                                           L O O P                                               *
//**************************************************************************************************
// Main loop of the program.                                                                       *
//**************************************************************************************************
void loop() {
  static bool wasConnected = NetworkFound; // Initialzustand merken, 10sec adaption

  handleSleepTimer();
  mp3loop();     // Do mp3 related actions
  if (updatereq) // Software update requested?
  {
    if (displaytype == T_NEXTION) // NEXTION in use?
    {
      update_software("lstmodn", // update NEXTION image from remote image
                      UPDATEHOST, TFTFILE);
    }
    update_software("lstmods", // Update sketch from remote file
                    UPDATEHOST, BINFILE);
    resetreq = true; // And reset
  }

  // WiFi-Reconnect-Logik, 10sec adaption
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnectAttempt >= WIFI_RECONNECT_INTERVAL) {
      lastReconnectAttempt = millis();
      dbgprint("Attempting WiFi reconnection...");
      NetworkFound = connectwifi();
      if (NetworkFound) {
        // Bei erfolgreicher Verbindung notwendige Services neu starten
        cmdserver.begin();
        ArduinoOTA.begin();
        MDNS.begin(NAME);
        dbgprint("HTTP server restarted after WiFi reconnect");
      }
    }
    wasConnected = false;
  } else if (!wasConnected) {
    // Bei wiederhergestellter Verbindung
    wasConnected = true;
    ipaddress = WiFi.localIP().toString();
    cmdserver.begin();
    dbgprint("Reconnected to WiFi. IP: %s", ipaddress.c_str());
  }

  if (resetreq) // Reset requested?
  {
    delay(1000);   // wait some time
    ESP.restart(); // Reboot
  }
  scanserial();  // Handle serial input
  scanserial2(); // Handle serial input from NEXTION (if active)
  scandigital(); // Scan digital inputs
  // scanIR();                                        // See if IR input
  ArduinoOTA.handle(); // Check for OTA
  mp3loop();           // Do more mp3 related actions
  if (http_response_flag && !cmdclient.connected()) {
    recoverHttpServerState();
  }
  handlehttpreply();
  if (!cmdclient.connected() && !http_response_flag) {
    cmdclient = cmdserver.available(); // Accept a new client only when idle
  }
  if (cmdclient.connected() && !http_response_flag) {
    handlehttp();
  }

  nvsAutoSave();         // See if time to save settings
  updateEncoderSwitch(); // Poll encoder push button (debounced)
  checkRotaryEncoder();  // Check rotary encoder functions
}
