#ifndef APP_PROTOTYPES_H
#define APP_PROTOTYPES_H

#include <Arduino.h>

//**************************************************************************************************
// Forward declaration and prototypes of various functions.                                        *
//**************************************************************************************************
void renderTimeOnTFT(const char *str, uint16_t color = 0xFFFF);
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
void handleRenderData();
String nvsGetHostFromPref(int16_t preset);
String nvsGetHostFromPref();
void showTextOnTFT(uint16_t inx, const char *str);
void showTextOnTFT(uint16_t inx, String &str);
#if defined(BLUETFT)
void showTitleOnTFT(const char *str);
void showTitleOnTFT(String &str);
void showStationNameOnTFT(const char *str);
void showStationNameOnTFT(String &str);
void showShortStatusOnTFT(const char *str);
void showShortStatusOnTFT(String &str);
void showLegacyOnTFT(const char *str);
void showLegacyOnTFT(String &str);
#endif

#endif
