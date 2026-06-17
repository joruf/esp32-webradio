/**
 * @brief Extract stream title from metadata string.
 *
 * @param ml Metadata string
 * @param streamtitle Output buffer for extracted title
 * @param bufsize Size of the output buffer
 * @return true if stream title was found, false otherwise
 */
static bool extractStreamTitle(const char *ml, char *streamtitle, size_t bufsize) {
  char *p1;
  char *p2;

  if (!strstr(ml, "StreamTitle=")) {
    return false;
  }

  dbgprint("Streamtitle found, %d bytes", strlen(ml));
  dbgprint(ml);

  // Search for end of title
  p1 = (char *)ml + 12; // Begin of artist and title
  if ((p2 = strstr(ml, ";"))) {
    if (*p1 == '\'') { // Surrounded by quotes
      p1++;
      p2--;
    }
    *p2 = '\0'; // Strip the rest of the line
  }

  // Save last part of string as streamtitle. Protect against buffer overflow
  strncpy(streamtitle, p1, bufsize);
  streamtitle[bufsize - 1] = '\0';
  return true;
}

/**
 * @brief Format stream title with artist/title separator.
 *
 * @param streamtitle Input/output stream title buffer
 */
static void formatStreamTitle(char *streamtitle) {
  char *p1;
  char *p2;

  // Look for artist/title separator
  if ((p1 = strstr(streamtitle, " - "))) {
    p2 = p1 + 3; // 2nd part of text at this position

    if (displaytype == T_NEXTION) {
      *p1++ = '\\'; // Replace 3 characters by "\r"
      *p1++ = 'r';
    } else {
      *p1++ = '\n'; // Replace 3 characters by newline
    }

    // Remove leading space in title
    if (*p2 == ' ') {
      p2++;
    }
    strcpy(p1, p2); // Shift 2nd part of title
  }
}

/**
 * @brief Show artist and song title if present in metadata.
 *        Show always if full=true.
 *
 * @param ml Metadata string or plain text
 * @param full If true, show even without StreamTitle tag
 */
void showStreamDataOnTFT(const char *ml, bool full) {
  char streamtitle[150];

  if (extractStreamTitle(ml, streamtitle, sizeof(streamtitle))) {
    // Save for status request from browser
    icystreamtitle = streamtitle;
    formatStreamTitle(streamtitle);
    showTextOnTFT(1, streamtitle);
  } else if (full) {
    // Info probably from playlist
    strncpy(streamtitle, ml, sizeof(streamtitle));
    streamtitle[sizeof(streamtitle) - 1] = '\0';
    icystreamtitle = streamtitle;
    showTextOnTFT(1, streamtitle);
  } else {
    icystreamtitle = "";
  }
}

/**
 * @brief Check whether a playlist line is a plausible stream URL or host.
 *
 * @param line Playlist line without trailing newline
 * @return true if the line should be treated as a stream entry
 */
static bool isValidPlaylistEntry(const char *line) {
  size_t len;
  size_t i;
  bool hasDot = false;
  bool hasAlnum = false;

  if (line == nullptr) {
    return false;
  }

  len = strlen(line);
  if (len < 5) {
    return false;
  }

  if (line[0] == '<' || line[0] == '!' || line[0] == '#') {
    return false;
  }

  if (strncasecmp(line, "http://", 7) == 0 || strncasecmp(line, "https://", 8) == 0) {
    return true;
  }

  for (i = 0; i < len; i++) {
    char c = line[i];
    if (c == '<' || c == '>' || c == '{' || c == '}') {
      return false;
    }
    if (c == '.') {
      hasDot = true;
    }
    if (isalnum((unsigned char)c)) {
      hasAlnum = true;
    }
  }

  if (strstr(line, "localhost") != nullptr) {
    return true;
  }

  return hasAlnum && hasDot;
}

/**
 * @brief Apply an HTTP Location header value to the active host.
 *
 * @param location Location header value (with or without scheme)
 * @return true if host was updated
 */
static bool applyHttpLocationHeader(const String &location) {
  String target = location;
  target.trim();

  if (target.length() == 0) {
    return false;
  }

  if (target.startsWith("http://")) {
    host = target.substring(7);
    return true;
  }
  if (target.startsWith("https://")) {
    host = target.substring(8);
    return true;
  }

  host = target;
  return true;
}

/**
 * @brief Parse host string to extract components.
 *
 * @param host Full host string
 * @param hostwoext Output: host without extension
 * @param port Output: port number
 * @param extension Output: URL extension
 */
static void parseHostString(const String &host, String &hostwoext, uint16_t &port, String &extension) {
  int inx;

  hostwoext = host;
  port = 80;
  extension = "/";

  // Extract extension
  inx = host.indexOf("/");
  if (inx > 0) {
    extension = host.substring(inx);
    hostwoext = host.substring(0, inx);
  }

  // Extract port number
  inx = hostwoext.indexOf(":");
  if (inx >= 0) {
    port = host.substring(inx + 1).toInt();
    hostwoext = host.substring(0, inx);
  }
}

/**
 * @brief Handle M3U playlist request.
 */
static void handlePlaylistRequest() {
  playlist = host;
  datamode = PLAYLISTINIT;

  if (playlist_num == 0) {
    playlist_num = 1;
  }
  dbgprint("Playlist request, entry %d", playlist_num);
}

/**
 * @brief Send HTTP request to the web radio server.
 *
 * @param hostwoext Host without extension
 * @param port Port number
 * @param extension URL extension
 */
static void sendHttpRequest(const String &hostwoext, uint16_t port, const String &extension) {
  String auth = nvsGetValue("basicauth");

  if (auth != "") {
    auth = base64::encode(auth.c_str());
    auth = String("Authorization: Basic ") + auth + String("\r\n");
  }

  mp3client.print(String("GET ") + extension + String(" HTTP/1.1\r\n") + String("Host: ") + hostwoext + String("\r\n") +
                  String("Icy-MetaData:1\r\n") + auth + String("Connection: close\r\n\r\n"));
}

/**
 * @brief Connect to the Internet radio server specified by newpreset.
 *
 * @return true if connection successful, false otherwise
 */
bool connectToWebradioServer() {
  String hostwoext;
  uint16_t port = 80;
  String extension = "/";
  const char *s;

  stop_mp3client();
  dbgprint("Connect to new host %s", host.c_str());

  showTextOnTFT(0, "Webradio");
  showTimeOnTFT("");

  datamode = INIT;
  chunked = false;

  // Handle M3U playlist
  if (host.endsWith(".m3u")) {
    handlePlaylistRequest();
  }

  // Parse host components
  parseHostString(host, hostwoext, port, extension);

  s = dbgprint("Connect to %s on port %d, extension %s", hostwoext.c_str(), port, extension.c_str());
  showTextOnTFT(1, s);

  if (mp3client.connect(hostwoext.c_str(), port)) {
    dbgprint("Connected to server");
    sendHttpRequest(hostwoext, port, extension);
    return true;
  }

  dbgprint("Request %s failed!", host.c_str());
  if ((host.indexOf('<') >= 0) || (host.indexOf('>') >= 0)) {
    playlist_num = 0;
    datamode = STOPPED;
  }
  return false;
}

/**
 * @brief Process QDATA chunk type - send data to VS1053.
 */
static void processQDataChunk() {
  claimSPI("chunk");
  vs1053player->playChunk(inchunk.buf, sizeof(inchunk.buf));
  releaseSPI();
  totalcount += sizeof(inchunk.buf);
}

/**
 * @brief Process QSTARTSONG chunk type - start the player.
 */
static void processQStartSong() {
  claimSPI("startsong");
  vs1053player->startSong();
  releaseSPI();
}

/**
 * @brief Process QSTOPSONG chunk type - stop the player.
 */
static void processQStopSong() {
  claimSPI("stopsong");
  vs1053player->setVolume(0);
  vs1053player->stopSong();
  releaseSPI();
  vTaskDelay(500 / portTICK_PERIOD_MS);
}

/**
 * @brief Play stream data from input queue.
 *        Handle all I/O to VS1053B during normal playing.
 *        Handles display of text, time and volume on TFT as well.
 *
 * @param parameter Task parameter (unused)
 */
void playStreamFromQueue(void *parameter) {
  while (true) {
    if (xQueueReceive(dataqueue, &inchunk, 5)) {
      // Wait if FIFO is full
      while (!vs1053player->data_request()) {
        vTaskDelay(1);
      }

      // Select the chunk type
      switch (inchunk.datatyp) {
        case QDATA:
          processQDataChunk();
          break;
        case QSTARTSONG:
          processQStartSong();
          break;
        case QSTOPSONG:
          processQStopSong();
          break;
        default:
          break;
      }
    }
  }
}

/**
 * @brief Handle TFT update with SPI bus management.
 */
static void handleTftUpdate() {
  if (dsp_isUseSPI()) {
    claimSPI("hspectft");
  }

  if (tft) {
    handle_tft_txt();
    dsp_update();
  }

  if (dsp_isUseSPI()) {
    releaseSPI();
  }
}

/**
 * @brief Handle time and alarm display updates.
 */
static void handleTimeAndAlarmUpdate() {
  if (!time_req) {
    return;
  }

  getCurrentTime();
  handleRadioAlarm();
}

/**
 * @brief Handle VS1053 volume and tone settings.
 */
static void handleVs1053VolumeAndTone() {
  updateVolumeFade();

  claimSPI("hspec");
  vs1053player->setVolume(getOutputVolume());

  if (reqtone) {
    reqtone = false;
    vs1053player->setTone(ini_block.rtone);
  }

  releaseSPI();
}

/**
 * @brief Handle TFT info display updates.
 */
static void handleTftInfoDisplay() {
  if (time_req) {
    time_req = false;
    if (NetworkFound) {
      showTimeOnTFT(timetxt);
      displayVolumeOnTFT();
      displayBatteryChargeOnTFT();
    }
  }
}

/**
 * @brief Handle special (non-stream data) functions for spftask.
 *        Runs once per spftask iteration.
 */
void handleDisplayData() {
  // 1. Update TFT
  handleTftUpdate();

  // 2. Time and alarm calculation (no SPI required)
  handleTimeAndAlarmUpdate();

  // 3. VS1053 volume and tone settings
  handleVs1053VolumeAndTone();

  // 4. Update TFT info display
  handleTftInfoDisplay();
}