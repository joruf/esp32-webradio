#include "config/features_config.h"
#include "net/web_theme_serve.h"

/**
 * @brief Returns the content type of a file based on its file extension.
 *
 * @param filename The filename (e.g. "index.html")
 * @return String Content type (e.g. "text/html") or empty string for disallowed
 * files
 */
String getContentType(String filename) {
  if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".json"))
    return "application/json";
  else if (filename.endsWith(".mp3"))
    return "audio/mpeg";
  else if (filename.endsWith(".pw"))
    return ""; // Passwords are secret
  return "text/plain";
}

/**
 * @brief Handles a request for a web page from PROGMEM.
 *
 * Sends the requested page (e.g. index.html, config.html, CSS, JS)
 * to the connected client. For unknown or invalid pages, a 404 error is
 * returned.
 *
 * @param pagename Name of the requested page (e.g. "/", "index.html")
 * @note The page is read from PROGMEM (flash memory) and sent to the client
 *       in chunks of up to 1024 bytes.
 * @see getContentType()
 */
void handleWebpage(const String &pagename) {
  String ct; // Content type
  String etag;
  String lastModified;
  const char *p;
  int l;                   // Size of requested page
  int TCPCHUNKSIZE = 1024; // Max number of bytes per write

  dbgprint("FileRequest received %s", pagename.c_str());
  if (pagename.indexOf("favicon.ico") >= 0) {
    cmdclient.print("HTTP/1.1 204 No Content\r\n");
    cmdclient.print("Connection: close\r\n");
    cmdclient.print("Cache-Control: public, max-age=600\r\n\r\n");
    return;
  }
  ct = getContentType(pagename);      // Get content type
  if ((ct == "") || (pagename == "")) // Empty is illegal
  {
    cmdclient.println("HTTP/1.1 404 Not Found");
    cmdclient.println("");
    return;
  } else {
    // Files is in PROGMEM
    // if (pagename.indexOf ("index.html") >= 0 ) {
    if (pagename == "/" || pagename.indexOf("index.html") >= 0) {
      p = index_html;
      l = sizeof(index_html) - 1; // We have to reduce the length to get the right char length
    } else if (pagename.indexOf("mp3stream.html") >= 0) {
      p = mp3stream_html;
      l = sizeof(mp3stream_html) - 1;
    } else if (pagename.indexOf("sound.html") >= 0) {
      p = sound_html;
      l = sizeof(sound_html) - 1;
    } else if (pagename.indexOf("scheduler.html") >= 0) {
      p = scheduler_html;
      l = sizeof(scheduler_html) - 1;
    } else if (pagename.indexOf("display.html") >= 0) {
      p = display_html;
      l = sizeof(display_html) - 1;
    } else if (pagename.indexOf("ota.html") >= 0) {
      p = ota_html;
      l = sizeof(ota_html) - 1;
    } else if (pagename.indexOf("maintenance.html") >= 0) {
      p = maintenance_html;
      l = sizeof(maintenance_html) - 1;
    } else if (pagename.indexOf("config.html") >= 0) {
      p = config_html;
      l = sizeof(config_html) - 1;
    } else if (pagename.indexOf("firmware.html") >= 0) { // ota-update
      p = firmware_html;
      l = sizeof(firmware_html) - 1;
    } else if (pagename.indexOf("about.html") >= 0) {
      p = about_html;
      l = sizeof(about_html) - 1;
    } else if (pagename.indexOf("diagnostics.html") >= 0) {
      p = diagnostics_html;
      l = sizeof(diagnostics_html) - 1;
    } else if (pagename.indexOf("eventlog.html") >= 0) {
      p = eventlog_html;
      l = sizeof(eventlog_html) - 1;
    } else if (pagename.indexOf("app.css") >= 0 || pagename.indexOf("index.css") >= 0) {
      p = app_css;
      l = sizeof(app_css) - 1;
    } else if (pagename.indexOf("app.js") >= 0 || pagename.indexOf("interface.js") >= 0) {
      p = app_js;
      l = sizeof(app_js) - 1;
    } else if (pagename.indexOf("ota/version.json") >= 0) {
      p = ota_version_embedded_json;
      l = sizeof(ota_version_embedded_json) - 1;
      /*
      } else if (pagename.indexOf ("bootstrap.min.css") >= 0 ) {
        p = bootstrap_css;
        l = sizeof (bootstrap_css)-1;
      } else if (pagename.indexOf ("bootstrap.min.js") >= 0 ) {
        p = bootstrap_js;
        l = sizeof (bootstrap_js)-1;
      } else if (pagename.indexOf ("jquery.min.js") >= 0 ) {
        p = jquery_js;
        l = sizeof (jquery_js)-1;
      } else if (pagename.indexOf ("favicon.ico") >= 0 ) {
        p = (char*)favicon_ico;
        l = sizeof (favicon_ico)-1;
      */
    } else {
      p = "";
      l = sizeof("");
    }

    if (*p == '\n') { // If page starts with newline:
      p++;            // Skip first character
      l--;
    }

    dbgprint("Length of page is %d", l);
    lastModified = String(FIRMWARE);
    etag = httpBuildWeakEtag(pagename, expandedHtmlLength(p, l));
    if (httpIsAssetNotModified(etag, lastModified)) {
      httpSendNotModified(etag, lastModified, 600);
      cmdclient.flush();
      return;
    }
    cmdclient.print(httpheader(ct, 600, etag, lastModified)); // Send header (10 minutes cache)

    if (l > 0 && !streamWebPage(cmdclient, p, l, ct, isDarkModeActive(), TCPCHUNKSIZE)) {
      dbgprint("HTTP client disconnected while sending %s", pagename.c_str());
    }
    cmdclient.flush();
    // The HTTP response ends with another blank line:
    cmdclient.println();
    dbgprint("Response send");
  }
}

/**
 * @brief Decode percent-encoded sequences (%XX) in a value to plain bytes.
 *
 * Reverses one layer of the web UI's JavaScript encodeURIComponent(). In
 * contrast to form decoding, a literal '+' is preserved because
 * encodeURIComponent() never emits '+' as a space substitute (it uses %20).
 *
 * @param text Percent-encoded input string
 * @return String Decoded result string
 */
static String urlDecodeValue(const String &text) {
  String decoded;
  size_t i = 0;

  decoded.reserve(text.length());
  while (i < text.length()) {
    char c = text[i];
    if ((c == '%') && (i + 2 < text.length())) {
      char h1 = text[i + 1];
      char h2 = text[i + 2];
      if (isxdigit((unsigned char)h1) && isxdigit((unsigned char)h2)) {
        char tmp[3];
        tmp[0] = h1;
        tmp[1] = h2;
        tmp[2] = '\0';
        decoded += (char)strtol(tmp, nullptr, 16);
        i += 3;
        continue;
      }
    }
    decoded += c;
    i++;
  }

  return decoded;
}

/**
 * @brief Parses a command with a possible "=" separator (wrapper function).
 *
 * Searches for an equals sign in the command. If present, the command is split
 * into parameter and value and passed to the second overload.
 *
 * @param str Command in the format "parameter=value" or just "parameter"
 * @return const char* Response to the command (e.g. "Command accepted")
 * @see analyzeCmd(const char* par, const char* val)
 */
const char *analyzeCmd(const char *str) {
  char *value;     // Points to value after equalsign in command
  const char *res; // Result of analyzeCmd

  value = strstr(str, "="); // See if command contains a "="
  if (value) {
    *value = '\0';                    // Separate command from value
    res = analyzeCmd(str, value + 1); // Analyze command and handle it
    *value = '=';                     // Restore equal sign
  } else {
    res = analyzeCmd(str, "0"); // No value, assume zero
  }
  return res;
}

/**
 * @brief Analyzes and executes a command with a parameter.
 *
 * This function processes all HTTP and serial commands of the system.
 * Supported commands include:
 * - Volume control (volume, upvolume, downvolume)
 * - Station selection (preset, preset_XX, station)
 * - Tone control (toneha, tonehf, tonela, tonelf)
 * - Sleep timer (sleeptime)
 * - Mute (mute)
 * - Playback control (stop, resume)
 * - System commands (reset, update, debug, test)
 * - Time synchronization (clk_server, clk_offset, clk_dst)
 * - Battery calibration (bat0, bat100)
 * - WiFi networks (getnetworks)
 * - Alarm schedules (alrm_config)
 * - Backlight (backlight_on, backlight_off)
 *
 * @param par Parameter name (e.g. "volume", "preset", "station")
 * @param val Parameter value (e.g. "75", "12", "http://...")
 * @return const char* Response string for the client
 * @note For relative commands (up/down), the value is added/subtracted
 * accordingly
 * @note Commands marked with "*)" in the description are only meaningful during
 * initialization
 */
const char *analyzeCmd(const char *par, const char *val) {
  String argument;        // Argument as string
  String value;           // Value of an argument as a string
  int ivalue;             // Value of argument as an integer
  static char reply[180]; // Reply to client, will be returned
  uint8_t oldvol;         // Current volume
  bool relative;          // Relative argument (+ or -)
  String tmpstr;          // Temporary for value
  uint32_t av;            // Available in stream/file

  enableBacklight(true);             // Enable backlight of TFT
  strcpy(reply, "Command accepted"); // Default reply
  argument = String(par);            // Get the argument
  chomp(argument);                   // Remove comment and useless spaces
  if (argument.length() == 0)        // Lege commandline (comment)?
  {
    return reply; // Ignore
  }
  argument.toLowerCase();                 // Force to lower case
  value = String(val);                    // Get the specified value
  chomp(value);                           // Remove comment and extra spaces
  if (argument == "station") {            // Web UI sends the URL via encodeURIComponent()
    value = urlDecodeValue(value);        // Decode %XX back to a usable stream URL
  }
  ivalue = value.toInt();                 // Also as an integer
  ivalue = abs(ivalue);                   // Make positive
  relative = argument.indexOf("up") == 0; // + relative setting?
  dbgprint("API Request: ", argument);
  if (argument.indexOf("down") == 0) // - relative setting?
  {
    relative = true;  // It's relative
    ivalue = -ivalue; // But with negative value
  }
  if (value.startsWith("http://")) // Does (possible) URL contain "http://"?
  {
    value.remove(0, 7); // remove it
  }
  if (value.length()) {
    tmpstr = value;                     // Make local copy of value
    if (argument.indexOf("passw") >= 0) // Password in value?
    {
      tmpstr = String("*******"); // hide it
    }
    dbgprint("Command: %s with parameter %s", argument.c_str(), tmpstr.c_str());
  } else {
    dbgprint("Command: %s (without parameter)", argument.c_str());
  }
  if (argument.indexOf("volume") >= 0) // Volume setting
  {
    // Volume may be of the form "upvolume", "downvolume" or "volume" for
    // relative or absolute setting
    oldvol = getVolume(); // Get current volume
    if (relative)                       // + relative setting?
    {
      adjustVolume(ivalue);
      // ini_block.reqvol = oldvol + ivalue; // Up/down by 0.5 or more dB
    } else {
      setVolume(ivalue);
      // ini_block.reqvol = ivalue; // Absolute setting
    }
    if (ini_block.reqvol > 127) // Wrapped around?
    {
      setVolume(0);
      // ini_block.reqvol = 0; // keep at zero
    }
    if (ini_block.reqvol > 100) {
      setVolume(100);
      ini_block.reqvol = 100; // Limit to normal values
    }
    playSound(true);
    sprintf(reply, "Volume is now %d", // Reply new volume
            ini_block.reqvol);
  } else if (argument == "mute") // Mute/unmute request
  {
    if (datamode == STOPPED) {
      runStream(true, currentpreset); // Play stream
    } else {
      runStream(false); // Stop stream
    }
  } else if (argument.indexOf("ir_") >= 0)       // Ir setting?
  {                                              // Do not handle here
  } else if (argument.indexOf("sleeptime") >= 0 || argument.indexOf("snooze") >= 0) // Snooze timer
  {
    dbgprint("Snooze timer set");
    int mins = value.toInt();
    if (mins > 0) {
      sleeptimer_ms = (int32_t)mins * 60 * 1000;
      dbgprint("Snooze set to %d min", mins);
#if defined(BLUETFT)
      dspDashboardInvalidateSnooze();
      dspDashboardUpdateSnooze();
#endif
    } else {
      sleeptimer_ms = -1;
      dbgprint("Snooze off");
#if defined(BLUETFT)
      dspDashboardInvalidateSnooze();
      dspDashboardUpdateSnooze();
#endif
    }
  } else if (argument.indexOf("checkupdate") >= 0) {
    otaChecker.check(true);
    if (otaChecker.isUpdateAvailable()) {
      sprintf(reply, "Update available: %s", otaChecker.latestVersion().c_str());
    } else {
      strcpy(reply, "Firmware is up to date");
    }
  } else if (argument.indexOf("preset_") >= 0) // Enumerated preset?
  {                                            // Do not handle here
  } else if (argument.indexOf("preset") >= 0)  // (UP/DOWN)Preset station?
  {
    // If MP3 player is active: change track
    if (localfile && ((datamode & DATA) != 0) && // MP3 player active?
        relative) {
      datamode = STOPREQD; // Force stop MP3 player
      tmpstr = selectnextSDnode(SD_currentnode,
                                ivalue); // Select the next or previous file on SD
      host = getSDfilename(tmpstr);
      hostreq = true;              // Request this host
      sprintf(reply, "Playing %s", // Reply new filename
              host.c_str());
    } else {
      if (relative) // Relative argument?
      {
        currentpreset = ini_block.newpreset; // Remember currentpreset
        ini_block.newpreset += ivalue;       // adjust currentpreset
      } else {
        ini_block.newpreset = ivalue; // Otherwise set station
        playlist_num = 0;             // Absolute, reset playlist
        currentpreset = -1;           // Make sure current is different
      }
      datamode = STOPREQD;               // Force stop MP3 player
      sprintf(reply, "Preset is now %d", // Reply new preset
              ini_block.newpreset);
    }
  } else if (argument == "resume" || argument == "play") // Start playback
  {
    if (datamode == STOPPED) {
      runStream(true, currentpreset);
      strcpy(reply, "Player started");
    } else {
      strcpy(reply, "Player already running");
    }
  } else if (argument == "stop") // Stop playback
  {
    if (datamode != STOPPED) {
      runStream(false);
      strcpy(reply, "Player stopped");
    } else {
      strcpy(reply, "Player already stopped");
    }
  } else if ((value.length() > 0) && ((argument == "mp3track") || // Select a track from SD card?
                                      (argument == "station")))   // Station in the form address:port
  {
    if (argument.startsWith("mp3")) // MP3 track to search for
    {
      if (!SD_okay) // SD card present?
      {
        strcpy(reply, "Command not accepted!"); // Error reply
        return reply;
      }
      value = getSDfilename(value); // like "localhost/........"
    }
    if (datamode & (HEADER | DATA | METADATA | PLAYLISTINIT | PLAYLISTHEADER | PLAYLISTDATA)) {
      datamode = STOPREQD; // Request STOP
    }
    host = value;   // Save it for storage and selection later
    hostreq = true; // Force this station as new preset
    sprintf(reply,
            "Playing %s", // Format reply
            host.c_str());
    utf8ascii(reply);              // Remove possible strange characters
  } else if (argument == "status") // Status request
  {
    if (datamode == STOPPED) {
      sprintf(reply, "Player stopped"); // Format reply
    } else {
      String stationName = metaGetStation();
      String songTitle = metaGetTitle();
      sprintf(reply, "%s - %s", stationName.c_str(),
              songTitle.c_str()); // Streamtitle from metadata
    }
  } else if (argument.startsWith("reset")) // Reset request
  {
    eventLogSetShutdownReason("user request"); // Record why we restart
    resetreq = true;                           // Reset all
  } else if (argument.startsWith("update")) // Update request
  {
    otaChecker.check(true);
    pendingOtaBinUrl = ""; // Latest release
    updatereq = true;
  } else if (argument == "installversion") // Install a specific (possibly older) release
  {
    otaChecker.check(false); // Ensure a manifest is loaded (embedded fallback)
    String binUrl = otaChecker.binUrlForVersion(value);
    if (binUrl.length() > 0) {
      pendingOtaBinUrl = binUrl;
      updatereq = true;
      snprintf(reply, sizeof(reply), "Installing firmware %s", value.c_str());
    } else {
      snprintf(reply, sizeof(reply), "Unknown firmware version: %s", value.c_str());
    }
  } else if (argument == "test") // Test command
  {
    if (localfile) {
      av = mp3filelength; // Available bytes in file
    } else {
      av = mp3client.available(); // Available in stream
    }
    sprintf(reply, "Free memory is %d, chunks in queue %d, stream %d, bitrate %d kbps", ESP.getFreeHeap(),
            uxQueueMessagesWaiting(dataqueue), av, mbitrate);
    dbgprint("Stack maintask is %d", uxTaskGetStackHighWaterMark(maintask));
    dbgprint("Stack playStreamFromQueue is %d", uxTaskGetStackHighWaterMark(xplayStreamFromQueue));
    dbgprint("Stack spftask  is %d", uxTaskGetStackHighWaterMark(xspftask));
    dbgprint("ADC reading is %d", adcval);
    dbgprint("scaniocount is %d", scaniocount);
    dbgprint("Max. mp3_loop duration is %d", max_mp3loop_time);
    max_mp3loop_time = 0; // Start new check
  }

  // Alarm clock schedules (JSON object)
  else if (argument == "alrm_config") {
    applyAlarmConfig(value);
    dbgprint("Alarm configuration updated");
  }

  // Legacy alarm keys (migrated automatically on boot)
  else if (argument == "alrm_start") {
    dbgprint("Legacy alrm_start ignored, use alrm_config");
  } else if (argument == "alrm_stop") {
    dbgprint("Legacy alrm_stop ignored, use alrm_config");
  }

  else if (argument.startsWith("backlight")) // Backlight ON/OFF
  {
    if (argument.indexOf("_on") >= 0) {
      enableBacklight(true);
    } else if (argument.indexOf("_off") >= 0) {
      enableBacklight(false);
    }
  }

  // Commands for bass/treble control
  else if (argument.startsWith("tone")) // Tone command
  {
    if (argument.indexOf("ha") > 0) // High amplitude? (for treble)
    {
      ini_block.rtone[0] = ivalue; // prepare to set ST_AMPLITUDE
    }
    if (argument.indexOf("hf") > 0) // High frequency? (for treble)
    {
      ini_block.rtone[1] = ivalue; // prepare to set ST_FREQLIMIT
    }
    if (argument.indexOf("la") > 0) // Low amplitude? (for bass)
    {
      ini_block.rtone[2] = ivalue; // prepare to set SB_AMPLITUDE
    }
    if (argument.indexOf("lf") > 0) // Low frequency? (for bass)
    {
      ini_block.rtone[3] = ivalue; // prepare to set SB_FREQLIMIT
    }
    reqtone = true; // Set change request
    sprintf(reply, "Parameter for bass/treble %s set to %d", argument.c_str(), ivalue);
  } else if (argument == "rate") // Rate command?
  {
    if (vs1053Available && vs1053player) {
      vs1053player->AdjustRate(ivalue); // adjust
    }
  }

  else if (argument == "debug") // debug on/off request?
  {
    DEBUG = ivalue;                     // set flag accordingly
  } else if (argument == "getnetworks") // List all WiFi networks?
  {
    sprintf(reply, networks.c_str());     // Reply is SSIDs
  } else if (argument.startsWith("clk_")) // TOD parameter?
  {
    bool restartNtp = false;
    if (argument.indexOf("server") > 0) // NTP server spec?
    {
      ini_block.clk_server = value; // set server
      restartNtp = true;
    }
    if (argument.indexOf("offset") > 0) // Offset with respect to UTC spec?
    {
      ini_block.clk_offset = value.toInt(); // set offset
    }
    if (argument.indexOf("dst") > 0) // Offset during DST spec?
    {
      ini_block.clk_dst = value.toInt(); // set DST offset
    }
    applyClockConfiguration(restartNtp);
    if (timeinfo.tm_year) {
      sprintf(reply, "Clock updated: %s", timetxt);
    } else if (!NetworkFound) {
      strcpy(reply, "Clock settings saved (offline, time not available)");
    } else {
      strcpy(reply, "Clock settings saved, waiting for NTP sync");
    }
  } else if (argument.startsWith("bat")) // Battery ADC value?
  {
    if (argument.indexOf("100") == 3) // 100 percent value?
    {
      ini_block.bat100 = ivalue;           // set it
    } else if (argument.indexOf("0") == 3) // 0 percent value?
    {
      ini_block.bat0 = ivalue; // set it
    }
  } else {
    sprintf(reply, "%s called with illegal parameter: %s", NAME, argument.c_str());
  }
  return reply; // Return reply to the caller
}