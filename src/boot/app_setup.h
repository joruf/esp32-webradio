#ifndef APP_SETUP_H
#define APP_SETUP_H

#include <esp_ota_ops.h>

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

  Serial.begin(115200); // For debug
  Serial.println();
  dbgprint("========================================");
  dbgprint("BOOT: ESP32 Webradio starting");
  dbgprint("BOOT: Firmware %s", FIRMWARE);
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
  otaChecker.check(false);
  loadAlarmConfigFromNvs(); // Load or migrate alarm JSON configuration
  dbgprint("BOOT: Preferences loaded, active preset %d, volume %d", ini_block.newpreset, ini_block.reqvol);
  // tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, NAME);
  WiFi.setHostname(NAME);

  // Initialize VS1053 player
  vs1053player->begin();
  if (vs1053player->isReady()) {
    uint16_t modeReg = vs1053player->read_register(0x00); // 0x00 ist SCI_MODE

    if (modeReg == 0x0000 || modeReg == 0xFFFF) {
      dbgprint("VS1053 not properly installed! Register-Dump: 0x%04X", modeReg);
      vs1053Available = false;
    } else {
      dbgprint("VS1053 erfolgreich initialisiert. Mode-Register: 0x%04X", modeReg);
      vs1053Available = true;
    }
  } else {
    dbgprint("VS1053 not detected, continuing without audio decoder");
    vs1053Available = false;
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


#endif
