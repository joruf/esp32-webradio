#ifndef GLOBALS_H
#define GLOBALS_H

#include "globals/types.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <driver/timer.h>

class VS1053;

#define otaclient mp3client

#ifndef sv
#define sv DRAM_ATTR static volatile
#endif

extern alarmStatus radioAlarmStatusChanged;
extern bool vs1053Available;
extern const char *msgPlayerStopped;
extern const char *msgTurnToSelectStation;
extern const char *msgPresetSelectTft;
extern const char *msgRadioAlarmOn;
extern const char *msgRadioAlarmOff;

extern int DEBUG;
extern int numSsid;
extern WiFiMulti wifiMulti;
extern ini_struct ini_block;
extern WiFiServer cmdserver;
extern WiFiClient mp3client;
extern WiFiClient cmdclient;
extern HardwareSerial *nxtserial;
extern TaskHandle_t maintask;
extern TaskHandle_t xplayStreamFromQueue;
extern TaskHandle_t xmp3StreamTask;
extern TaskHandle_t xspftask;
extern SemaphoreHandle_t SPIsem;
extern SemaphoreHandle_t metaMutex; // Guards the shared metadata strings (icyname/icystreamtitle)
extern hw_timer_t *timer;
extern char timetxt[40];
extern char cmd[130];
extern uint8_t tmpbuff[6000];
extern QueueHandle_t dataqueue;
extern QueueHandle_t spfqueue;
extern qdata_struct outchunk;
extern qdata_struct inchunk;
extern uint8_t *outqp;
extern uint32_t totalcount;
extern datamode_t datamode;
extern int metacount;
extern int datacount;
extern char metalinebf[METASIZ + 1];
extern int16_t metadataLineIndex;
extern String icystreamtitle;
extern String icyname;

/**
 * @brief Thread-safe setter for the current song title (icystreamtitle).
 *
 * The stream producer task writes the metadata while the UI/display tasks read
 * it. Direct String assignment across tasks can free a heap buffer that another
 * task is currently reading, so all cross-task access is serialized here.
 *
 * @param value New song title (nullptr is treated as an empty string)
 * @return void
 */
void metaSetTitle(const char *value);

/**
 * @brief Thread-safe getter returning a private copy of the song title.
 *
 * @return String Copy of icystreamtitle taken under the metadata mutex
 */
String metaGetTitle();

/**
 * @brief Thread-safe setter for the current station name (icyname).
 *
 * @param value New station name (nullptr is treated as an empty string)
 * @return void
 */
void metaSetStation(const char *value);

/**
 * @brief Thread-safe getter returning a private copy of the station name.
 *
 * @return String Copy of icyname taken under the metadata mutex
 */
String metaGetStation();
extern String ipaddress;
extern int bitrate;
extern int mbitrate;
extern int metaint;
extern int16_t currentpreset;
extern String host;
extern String playlist;
extern bool hostreq;
extern bool reqtone;
extern bool muteflag;
extern volatile bool resetreq;        // Request to reset the ESP32
extern bool updatereq;
extern volatile bool otaActive;       // True while OTA/firmware update is running
extern bool NetworkFound;
extern String networks;
extern int16_t playlist_num;
extern File mp3file;
extern uint32_t mp3filelength;
extern bool localfile;
extern bool chunked;
extern int chunkcount;
extern String http_getcmd;
extern String http_rqfile;
extern bool http_response_flag;
extern uint32_t http_post_content_length;
extern uint16_t ir_value;
extern uint32_t ir_0;
extern uint32_t ir_1;
extern struct tm timeinfo;
extern bool time_req;
extern bool SD_okay;
extern String SD_nodelist;
extern int SD_nodecount;
extern String SD_currentnode;
extern uint16_t adcval;
extern uint32_t clength;
extern uint32_t max_mp3loop_time;
extern int16_t scanios;
extern int16_t scaniocount;
extern uint16_t bltimer;
extern display_t displaytype;
extern std::vector<WifiInfo_t> wifilist;
extern nvs_page nvsbuf;
extern const esp_partition_t *nvs;
extern esp_err_t nvserr;
extern uint32_t nvshandle;
extern uint8_t namespace_ID;
extern char nvskeys[MAXKEYS][16];
extern std::vector<keyname_t> keynames;

extern DRAM_ATTR volatile int16_t rotationcount;
extern DRAM_ATTR volatile uint16_t enc_inactivity;
extern DRAM_ATTR volatile bool singleclick;
extern DRAM_ATTR volatile bool doubleclick;
extern DRAM_ATTR volatile bool longclick;
extern enc_menu_t enc_menu_mode;

extern progpin_struct progpin[];
extern touchpin_struct touchpin[];

extern uint32_t lastReconnectAttempt;
extern VS1053 *vs1053player;

#endif
