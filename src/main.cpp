// filename: ESP32_Webradio.ino
//  Board > esp32 > ESP32 Dev Module
//  Sollte der Upload abbrechen mit "Fehlgeschlagenes Hochladen: Hochladefehler: exit status 2", den USB-Stecker
//  ausstecken, auf Hochladen klicken etwas warten bis "Kompiliere Scatch" bei 3/4 steht und dann USB wieder einstecken
//  während des ende der Kompilierungsphase BOARD-VERWALTUNG: esp32 (Espressif Systems) v3.3.0 Space overlaod>100% -->
//  Tools > Partition Scheme > custom ota_app (dual OTA slots, see partitions/ota_app.csv)

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

#include "config/firmware_config.h"
#include "globals/types.h"
#include "globals/app_prototypes.h"
#include "globals/globals.h"

#include <PubSubClient.h>
#include <ESPmDNS.h>
#include <time.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
#include <driver/adc.h>
#include <Update.h>
#include <base64.h>
#include <nvs.h>
#include <nvs_flash.h>

//**************************************************************************************************
//     Web UI assets (PROGMEM).                                                                  *
//**************************************************************************************************
#include "web_assets/web_assets.h"
#include "defaultprefs.h"
//**************************************************************************************************
//     VS1053 stuff.                                                                               *
//**************************************************************************************************
#include "handler/vs1053.h"
#include "handler/nvs.h"
#include "handler/string.h"
#include "handler/isr.h"
#include "handler/volume.h"
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
#include "handler/spi.h"
#include "handler/helper.h"
#include "handler/alarm_config.h"
#include "handler/radio_config.h"
#include "handler/captive_portal.h"
#include "handler/sd.h"
#include "handler/alarm.h"
#include "handler/loop.h"
#include "handler/wifi_signal.h"
#include "handler/setup.h"
#include "handler/xml.h"
#include "handler/time.h"
#include "handler/title_history.h"
#include "handler/features_config.h"
#include "handler/vu_meter.h"
#include "handler/stream_watchdog.h"
#include "handler/ota_check.h"
#include "handler/upgrade.h"
#include "handler/live_status.h"
#include "handler/wifi_display.h"
#include "handler/http.h"
#include "handler/rotary_encoder.h"
#include "handler/encoder_switch.h"
#include "handler/interface.h"
#include "handler/display.h"
#include "handler/stream.h"
#include "handler/mp3.h"
#include "writeDefaultConfig.h"
#include "boot/boot_validation.h"
#include "boot/app_setup.h"

void setup() { radioSetup(); }

void loop() {
  static bool wasConnected = NetworkFound; // Initialzustand merken, 10sec adaption

  handleSleepTimer();
  mp3loop();     // Do mp3 related actions
  if (updatereq) // Software update requested?
  {
    updatereq = false;
    bool updated = false;

    enableBacklight(true);
    otaCheckForUpdate(true);

    if (displaytype == T_NEXTION) // NEXTION in use?
    {
      updated = update_software("lstmodn", // update NEXTION image from remote image
                                UPDATEHOST, TFTFILE);
    } else {
      if (otaChecker.latestBinUrl().length() > 0) {
        updated = updateFirmwareFromUrl(otaChecker.latestBinUrl());
      }
      if (!updated) {
        updated = update_software("lstmods", // Update sketch from remote file
                                  UPDATEHOST, BINFILE);
      }
    }

    if (updated) {
      dspOtaDisplaySuccess();
      delay(3000);
      resetreq = true;
    } else {
      dbgprint("OTA update failed, device not restarted");
      delay(5000);
      dspOtaDisplayRestoreDashboard();
    }
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
  if (http_response_flag && !cmdclient.connected() && !sseStreamActive) {
    recoverHttpServerState();
  }
  handlehttpreply();
  httpPromotePending(cmdclient, http_response_flag);
  httpAcceptPending(cmdserver, cmdclient, http_response_flag);
  if (sseStreamActive) {
    httpServiceSse();
  } else if (!cmdclient.connected() && !http_response_flag) {
    cmdclient = cmdserver.available(); // Accept a new client only when idle
  }
  if (cmdclient.connected() && !http_response_flag) {
    handlehttp();
  }
  captivePortal.loop();
  streamWatchdog.poll();
  vuMeter.update();

  nvsAutoSave();         // See if time to save settings
  updateEncoderSwitch(); // Poll encoder push button (debounced)
  checkRotaryEncoder();  // Check rotary encoder functions
}
