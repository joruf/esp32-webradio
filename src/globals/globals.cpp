#include "globals/globals.h"

alarmStatus radioAlarmStatusChanged = ALRM_NONE;
bool vs1053Available = false;
const char *msgPlayerStopped = "Player stopped!\nclick\nto select new station";
const char *msgTurnToSelectStation = "Turn to select station\nPress to confirm";
const char *msgPresetSelectTft = "Drehen=Sender, Klick=OK";
const char *msgRadioAlarmOn = "Radio Alarm\nhas turned the music ON";
const char *msgRadioAlarmOff = "Radio Alarm\nhas turned the music OFF";

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
TaskHandle_t xmp3StreamTask;       // Task handle for the stream producer task
TaskHandle_t xspftask;             // Task handle for special functions
SemaphoreHandle_t SPIsem = NULL;   // For exclusive SPI usage
SemaphoreHandle_t metaMutex = NULL; // Serializes access to the shared metadata strings
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
int16_t metadataLineIndex;               // Index for metalinebf

String icystreamtitle; // Streamtitle from metadata --> Song title
String icyname;        // Icecast station name --> SWR3,...

/**
 * @brief Thread-safe setter for the current song title (icystreamtitle).
 *
 * @param value New song title (nullptr is treated as an empty string)
 * @return void
 */
void metaSetTitle(const char *value) {
  if (metaMutex) {
    xSemaphoreTake(metaMutex, portMAX_DELAY);
  }
  icystreamtitle = value ? value : "";
  if (metaMutex) {
    xSemaphoreGive(metaMutex);
  }
}

/**
 * @brief Thread-safe getter returning a private copy of the song title.
 *
 * @return String Copy of icystreamtitle taken under the metadata mutex
 */
String metaGetTitle() {
  String result;
  if (metaMutex) {
    xSemaphoreTake(metaMutex, portMAX_DELAY);
  }
  result = icystreamtitle;
  if (metaMutex) {
    xSemaphoreGive(metaMutex);
  }
  return result;
}

/**
 * @brief Thread-safe setter for the current station name (icyname).
 *
 * @param value New station name (nullptr is treated as an empty string)
 * @return void
 */
void metaSetStation(const char *value) {
  if (metaMutex) {
    xSemaphoreTake(metaMutex, portMAX_DELAY);
  }
  icyname = value ? value : "";
  if (metaMutex) {
    xSemaphoreGive(metaMutex);
  }
}

/**
 * @brief Thread-safe getter returning a private copy of the station name.
 *
 * @return String Copy of icyname taken under the metadata mutex
 */
String metaGetStation() {
  String result;
  if (metaMutex) {
    xSemaphoreTake(metaMutex, portMAX_DELAY);
  }
  result = icyname;
  if (metaMutex) {
    xSemaphoreGive(metaMutex);
  }
  return result;
}

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
volatile bool resetreq = false;        // Request to reset the ESP32
bool updatereq = false;                // Request to update software from remote host
String pendingOtaBinUrl;               // Specific firmware bin URL to install (e.g. chosen older release)
volatile bool otaActive = false;       // True while OTA/firmware update is running
bool NetworkFound = false;             // True if WiFi network connected
bool wifiConfigApActive = false;       // True while configuration soft-AP is active
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
// Rotary encoder (DRAM_ATTR for ISR access from other translation units)
DRAM_ATTR volatile int16_t rotationcount = 0;
DRAM_ATTR volatile uint16_t enc_inactivity = 0;
DRAM_ATTR volatile bool singleclick = false;
DRAM_ATTR volatile bool doubleclick = false;
DRAM_ATTR volatile bool longclick = false;
enc_menu_t enc_menu_mode = VOLUME;

progpin_struct progpin[] =
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

touchpin_struct touchpin[] =
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

VS1053 *vs1053player = nullptr;
uint32_t lastReconnectAttempt = 0;