#ifndef APP_SETUP_H
#define APP_SETUP_H

#include <esp_ota_ops.h>
#include <esp_task_wdt.h>
#include "config/gpio_valid.h"
#include "boot/st7701_boot_wdt.h"
#if defined(ST7701_28B)
#include "display/st7701_board_early.h"
#include "input/St7701Gt911Touch.h"
#include "audio/mcp23017_vs1053.h"
extern St7701Gt911Touch st7701Touch;
#endif
#if defined(RADIO_I2S_A2DP_TX)
#include "audio/radio_i2s_a2dp.h"
#endif

void radioSetup() {
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

  Serial.begin(115200); // For debug (USB CDC on S3 with ARDUINO_USB_MODE=1)
#if defined(ARDUINO_USB_CDC_ON_BOOT) && (ARDUINO_USB_CDC_ON_BOOT)
  delay(800); // Allow host to re-enumerate CDC after reset
#endif
  Serial.println();
  Serial.println("ESP32-Webradio boot");
  Serial.println();
#if defined(ST7701_28B)
  st7701SilenceOnboardBuzzer();
  dbgprint("BOOT: Onboard buzzer silenced (TCA9554 EXIO8)");
#endif
  dbgprint("========================================");
  dbgprint("BOOT: ESP32 Webradio starting");
  dbgprint("BOOT: Firmware %s", FIRMWARE);
  dbgprint("BOOT: Hardware profile default %s, active %s", HW_PROFILE_DEFAULT,
           hardwareProfileActiveId().c_str());
  dbgprint("BOOT: CPU core %d at %d MHz", xPortGetCoreID(), ESP.getCpuFreqMHz());
  dbgprint("BOOT: Free heap %d bytes", ESP.getFreeHeap());
  const esp_partition_t *otaSlot = esp_ota_get_next_update_partition(NULL);
  if (otaSlot) {
    dbgprint("BOOT: OTA slot available, size %u bytes, sketch %u bytes", (unsigned)otaSlot->size,
             (unsigned)ESP.getSketchSize());
  } else {
    dbgprint("BOOT: WARNING - no OTA slot in partition table, firmware updates will fail");
  }
  dbgprint("========================================");

#if defined(ST7701_28B)
  {
    esp_task_wdt_config_t wdtCfg = {
        .timeout_ms = 30000,
        .idle_core_mask = (1 << 0) | (1 << 1),
        .trigger_panic = true,
    };
    esp_task_wdt_reconfigure(&wdtCfg);
    esp_task_wdt_add(NULL);
    dbgprint("BOOT: Task WDT timeout extended to %u ms (ST7701 RGB bring-up)", (unsigned)wdtCfg.timeout_ms);
  }
#endif

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
#if defined(ST7701_28B)
  dbgprint(dtyp, "ST7701_28B");
#endif
#if defined(LCD1602I2C)
  dbgprint(dtyp, "LCD1602");
#endif
#if defined(NEXTION)
  dbgprint(dtyp, "NEXTION");
#endif

  maintask = xTaskGetCurrentTaskHandle(); // My taskhandle
  SPIsem = xSemaphoreCreateMutex();
  ;                                 // Semaphore for SPI bus
  metaMutex = xSemaphoreCreateMutex(); // Serializes cross-task metadata access
  eventLogInitMutex();                 // Protects the runtime event-log buffer
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
  dbgprint("BOOT: GPIO scan (Enc %d/%d/%d, VS CS/DCS/DREQ %d/%d/%d, SPI %d/%d/%d)",
           ini_block.enc_clk_pin, ini_block.enc_dt_pin, ini_block.enc_sw_pin, ini_block.vs_cs_pin,
           ini_block.vs_dcs_pin, ini_block.vs_dreq_pin, ini_block.spi_sck_pin, ini_block.spi_miso_pin,
           ini_block.spi_mosi_pin);
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
  // Classic ESP32 only: sample free progpin pull-ups at boot. Skipped on S3 because
  // the WROOM pin table includes GPIOs that do not exist or are board-fixed (RGB/PSRAM).
  for (i = 0; (pinnr = progpin[i].gpio) >= 0; i++) // Check programmable input pins
  {
    if (!gpioIsValid(pinnr)) {
      continue;
    }
    if (progpin[i].reserved) {
      continue; // Skip SPI/VS/TFT/SD/PSRAM and other claimed pins
    }
    // Input-only RTC pins have no internal pull-up; avoid GPIO driver errors.
    if (pinnr >= 34) {
      pinMode(pinnr, INPUT);
    } else {
      pinMode(pinnr, INPUT_PULLUP); // Input for control button
    }
    delay(10);
    // Check if pull-up active
    if ((progpin[i].cur = digitalRead(pinnr)) == HIGH) {
      p = "HIGH";
    } else {
      p = "LOW, probably no PULL-UP"; // No Pull-up
    }
    dbgprint("GPIO%d is %s", pinnr, p);
  }
#else
  dbgprint("BOOT: Progpin pull-up scan skipped on ESP32-S3");
#endif

  readprogbuttons(); // Program the free input pins
#if defined(ST7701_28B)
  st7701FeedTaskWdt();
  dbgprint("BOOT: GT911 reset deferred until after ST7701 display (shared TCA9554 I2C)");
#endif

  if (vs1053SpiBusConfigured(ini_block.vs_cs_pin, ini_block.vs_dcs_pin, ini_block.vs_dreq_pin, ini_block.spi_sck_pin,
                             ini_block.spi_miso_pin, ini_block.spi_mosi_pin)) {
#if defined(ST7701_28B)
    dbgprint("BOOT: VS1053 SPI deferred until after network (USB D+/D- shared SPI)");
#else
    const int8_t spiMiso =
        (ini_block.spi_miso_pin >= 0) ? ini_block.spi_miso_pin : (int8_t)(-1);
    SPI.begin(ini_block.spi_sck_pin, spiMiso,
              ini_block.spi_mosi_pin); // Init VSPI bus with default or modified pins
#endif
  } else {
#if defined(ST7701_28B)
    dbgprint("BOOT: VS1053 SPI skipped (incomplete or unsafe pins on S3 Touch 2.8B)");
#else
    dbgprint("SPI bus deferred (VS1053 CS/DCS/DREQ not assigned)");
#endif
  }
  vs1053player = new VS1053(ini_block.vs_cs_pin, ini_block.vs_dcs_pin, ini_block.vs_dreq_pin, ini_block.vs_shutdown_pin,
                            ini_block.vs_shutdownx_pin); // Make instance of player
  if (ini_block.ir_pin >= 0) {
    dbgprint("Enable pin %d for IR", ini_block.ir_pin);
    pinMode(ini_block.ir_pin, INPUT);                  // Pin for IR receiver VS1838B
    attachInterrupt(ini_block.ir_pin, isr_IR, CHANGE); // Interrupts will be handle by isr_IR
  }

#if defined(ST7701_28B)
  const bool displayConfigured = true;
#else
  const bool displayConfigured = (ini_block.tft_cs_pin >= 0) || (ini_block.tft_scl_pin >= 0);
#endif
  if (displayConfigured) {
    dbgprint("Start display");
    Serial.println("Start display (ST7701/TFT)");
    Serial.flush();
    if (dsp_begin()) {         // Init display
      dsp_setRotation();       // Use landscape format
      dsp_erase();             // Clear screen
#if defined(ST7701_28B)
      dsp_setTextWrap(false);
      dsp_setTextSize(2);      // Readable boot text on 640x480 landscape
      dsp_setTextColor(WHITE);
      dsp_setCursor(8, 24);
#else
      dsp_setTextSize(1);      // Small character font
      dsp_setTextColor(WHITE); // Info in white
      dsp_setCursor(0, 0);     // Top of screen
#endif

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

#if defined(ST7701_28B)
      st7701UiVolumeDirty = true; // Single RGB flush path: spftask only
#else
      dsp_update(); // Show on physical screen
#endif
    }
#if defined(ST7701_28B)
    if (!st7701Touch.begin()) {
      Serial.println("GT911: begin FAILED (swipe volume disabled until next boot fix)");
    }
#if defined(USE_MCP23017_VS1053) && !defined(RADIO_I2S_A2DP_TX)
    // Same I2C bus as TCA9554/GT911; address must be 0x27 (not 0x20).
    ini_block.enc_clk_pin = -1;
    ini_block.enc_dt_pin = -1;
    ini_block.enc_sw_pin = -1;
    ini_block.spi_sck_pin = 43;
    ini_block.spi_mosi_pin = 44;
    if (!mcp23017Vs1053Begin()) {
      // GPIO4 is battery ADC - avoid. CS0 / DCS16 / DREQ20(D+), write-only (no MISO).
      dbgprint("BOOT: MCP missing - VS1053 CS0/DCS16/DREQ20 write-only");
      ini_block.vs_cs_pin = 0;
      ini_block.vs_dcs_pin = 16;
      ini_block.vs_dreq_pin = 20;
      ini_block.spi_miso_pin = -1;
      if (vs1053player) {
        vs1053player->setPins(0, 16, 20);
      }
    } else {
      mcp23017Vs1053DeselectAll();
      // Full SPI: MISO=0 DREQ=16, CS/DCS via MCP (no GPIO4, no USB D-).
      ini_block.vs_cs_pin = VS1053_MCP_CS_SENTINEL;
      ini_block.vs_dcs_pin = VS1053_MCP_DCS_SENTINEL;
      ini_block.vs_dreq_pin = 16;
      ini_block.spi_miso_pin = 0;
      if (vs1053player) {
        vs1053player->setPins(VS1053_MCP_CS_SENTINEL, VS1053_MCP_DCS_SENTINEL, 16);
      }
      dbgprint("BOOT: MCP23017 ready - VS1053 MISO=0 DREQ=16 CS/DCS=MCP");
    }
#endif
#endif
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

  // SD card: SPI (external CS) or SD_MMC (ESP32-CAM onboard slot), per hardware profile
  if (hardwareProfileSdEnabled()) {
    const char *backend = hardwareProfileSdBackend();
    if (strcmp(backend, "mmc") == 0) {
      mountSDCardMmc();
    } else if (strcmp(backend, "spi") == 0 && ini_block.sd_cs_pin >= 0) {
      mountSDCard();
    } else {
      dbgprint("SD enabled in profile but backend '%s' not usable", backend);
    }
  } else {
    dbgprint("SD mount skipped (hardware profile)");
  }

  mk_lsan(); // Make a list of acceptable networks
             // in preferences.
  dbgprint("BOOT: %d WiFi credential(s) loaded from NVS", wifilist.size());
  WiFi.mode(WIFI_STA);    // This ESP is a station
  WiFi.persistent(false); // Do not save SSID and password
  WiFi.disconnect();      // After restart router could still
  delay(100);             // keep old connection
  listNetworks();         // Find WiFi networks
  readprefs(false);       // Read preferences
  setFeatureDefaults();
  {
    String cfg = nvsGetRadioConfig();
    if (cfg.length() > 0) {
      JsonDocument doc;
      if (!deserializeJson(doc, cfg)) {
        applyFeaturesFromDoc(doc);
      }
    }
  }
  // Dedicated feat_* keys are authoritative when the radio_config blob lags.
  applyFeatureNvsKeyOverrides();
  // Features may enable 180° landscape after the first dsp_setRotation() at boot.
  applyDisplayOrientation(false);
  otaChecker.check(false);
  loadAlarmConfigFromNvs(); // Load or migrate alarm JSON configuration
  syncAlarmsToRadioConfig(); // Heal a stale alarms copy in the radio_config blob
#if defined(ST7701_28B)
  if (ini_block.reqvol == 0) {
    ini_block.reqvol = RADIO_DEFAULT_VOLUME;
    nvsAddValue("volume", String(RADIO_DEFAULT_VOLUME));
    if (nvsCommit() == ESP_OK) {
      dbgprint("BOOT: Volume clamped 0 -> %d (ST7701 GT911 safety)", (int)RADIO_DEFAULT_VOLUME);
    }
  }
#endif
  dbgprint("BOOT: Preferences loaded, active preset %d, volume %d", ini_block.newpreset, ini_block.reqvol);
  // tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, NAME);
  WiFi.setHostname(NAME);

  // Bring up WiFi / SoftAP before VS1053 so a missing or hanging decoder cannot
  // block network provisioning (headless CAM without VS1053 / encoder).
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
    ArduinoOTA.onEnd([]() { // Push-style OTA finished; ESP restarts internally
      eventLogSetShutdownReason("firmware update (OTA push)");
      eventLogRecordShutdown();
    });
    ArduinoOTA.onError([](ota_error_t error) { otaActive = false; }); // Resume producer on failed push
    ArduinoOTA.begin(); // Allow update over the air

    if (MDNS.begin(NAME)) { // Start MDNS transponder
      MDNS.addService("http", "tcp", 80);
      MDNS.addServiceTxt("http", "tcp", "app", "esp32-webradio");
      MDNS.addService("webradio", "tcp", 80);
      dbgprint("BOOT: mDNS responder started as %s.local (http + webradio)", NAME);
    } else {
      dbgprint("BOOT: ERROR - mDNS responder setup failed");
    }
  } else {
    dbgprint("BOOT: Station mode unavailable, configuration AP or offline mode active");
    currentpreset = ini_block.newpreset; // No network: newpreset is -1, so means do not start radio
  }

#if defined(ST7701_28B)
  // VS1053 SPI on GPIO19/20 shares USB D-/D+; init from loop() after HTTP/WDT-safe boot.
  vs1053Available = false;
#if defined(RADIO_I2S_A2DP_TX)
  dbgprint("BOOT: RADIO_I2S_A2DP_TX: soft-decode -> I2S (skip VS1053 SPI on 43/44/16)");
  radioI2sA2dpBegin();
#else
  dbgprint("BOOT: VS1053 init deferred (ST7701: display + WiFi first)");
#endif
#else
  // Initialize VS1053 after network is up (optional hardware)
  vs1053player->begin();
  if (vs1053player->isReady()) {
    uint16_t modeReg = vs1053player->read_register(0x00);

    if (modeReg == 0x0000 || modeReg == 0xFFFF) {
      dbgprint("VS1053 not properly installed! Register-Dump: 0x%04X", modeReg);
      eventLogRecordError("VS1053 invalid mode register (check wiring)");
      vs1053Available = false;
    } else {
      dbgprint("VS1053 erfolgreich initialisiert. Mode-Register: 0x%04X", modeReg);
      vs1053Available = true;
    }
  } else {
    dbgprint("VS1053 not detected, continuing without audio decoder");
    eventLogRecordError("VS1053 audio decoder not detected");
    vs1053Available = false;
  }
#endif

  /*
  Legacy ESP32 Arduino 2.x timer setup (kept for reference):
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &timer100, true);
  timerAlarmWrite(timer, 100000, true);
  timerAlarmEnable(timer);
  */
  // ESP32 Arduino 3.x: 1 MHz timer, alarm every 100000 us (100 ms), autoreload.
  timer = timerBegin(1000000);
  timerAttachInterrupt(timer, &timer100);
  timerAlarm(timer, 100000, true, 0);
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
  if (gpioIsValid(ini_block.enc_clk_pin) && gpioIsValid(ini_block.enc_dt_pin)) {
    attachInterrupt(ini_block.enc_clk_pin, isr_enc_turn, CHANGE);
    attachInterrupt(ini_block.enc_dt_pin, isr_enc_turn, CHANGE);
    if (gpioIsValid(ini_block.enc_sw_pin)) {
      initEncoderSwitch(ini_block.enc_sw_pin);
    }
    dbgprint("Rotary encoder is enabled (clk/dt/sw=%d/%d/%d)", ini_block.enc_clk_pin, ini_block.enc_dt_pin,
             ini_block.enc_sw_pin);
  } else {
    dbgprint("Rotary encoder is disabled (%d/%d/%d)", ini_block.enc_clk_pin, ini_block.enc_dt_pin,
             ini_block.enc_sw_pin);
  }

  if (NetworkFound) {
    getCurrentTime(); // Sync time (write it in the register timetxt)
  }

  // Record this boot (and infer an unexpected shutdown if the last restart was
  // not a planned software restart). The timestamp is patched later by
  // eventLogProcess() once the NTP clock is available.
  eventLogRecordBoot();

  if (tft) {
    dsp_fillRect(0, 8, dsp_getwidth(), dsp_getheight() - 8, BLACK); // Clear most of the screen
  }

  outchunk.datatyp = QDATA; // This chunk dedicated to QDATA

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
  /*
  adc1_config_width(ADC_WIDTH_12Bit);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_0db);
  */
  adc1_config_width(ADC_WIDTH_BIT_12);                       // Correct Enum for deprecated function
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0); // New Enum Name
#endif

  dataqueue = xQueueCreate(QSIZ, sizeof(qdata_struct)); // Create queue for communication
#if !defined(ST7701_28B)
  if (vs1053Available) {
    dbgprint("▶️ VS1053 available. Start Audio-Task...");
    xTaskCreatePinnedToCore(playStreamFromQueue, "playStreamFromQueue", 4096, NULL, 3, &xplayStreamFromQueue, 0);
  } else {
    dbgprint("⚠️ VS1053 missing or invalid! Stream task runs without audio output.");
  }
#else
#if defined(RADIO_I2S_A2DP_TX)
  dbgprint("RADIO_I2S_A2DP: stream task = preset/host only; PCM via soft-decode I2S");
#else
  if (!vs1053Available) {
    dbgprint("⚠️ VS1053 pending deferred init; stream task runs without audio until ready");
  }
#endif
#endif

  // Producer: network/SD pump and stream watchdog (always, so UI/metadata work without VS1053).
  xTaskCreatePinnedToCore(mp3StreamTask,   // Task to fill the audio queue.
                          "mp3StreamTask", // name of task.
                          10240,           // Stack size (connect/playlist parsing use String)
                          NULL,            // parameter of the task
                          2,               // Above spftask so WiFi pump wins on core 1
                          &xmp3StreamTask, // Task handle
                          1);              // Run on CPU 1

#if defined(ST7701_28B)
  xTaskCreatePinnedToCore(spftask, "Spftask", 3072, NULL, 1, &xspftask, 0); // RGB dashboard on core 0
#else
  xTaskCreate(spftask, "Spftask", 2048, NULL, 1, &xspftask);
#endif
}


#endif
