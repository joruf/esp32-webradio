/**
 * @brief Locate the StreamTitle value in an ICY metadata block (case-insensitive).
 *
 * @param ml Metadata string
 * @return Pointer to first character after "StreamTitle=", or nullptr if not found
 */
static const char *findStreamTitleTag(const char *ml) {
  const char tag[] = "streamtitle=";
  const size_t taglen = sizeof(tag) - 1;

  if (ml == nullptr) {
    return nullptr;
  }

  for (const char *p = ml; *p; p++) {
    if (strncasecmp(p, tag, taglen) == 0) {
      return p + taglen;
    }
  }

  return nullptr;
}

/**
 * @brief Extract stream title from metadata string.
 *
 * @param ml Metadata string
 * @param streamtitle Output buffer for extracted title
 * @param bufsize Size of the output buffer
 * @return true if a non-empty stream title was found, false otherwise
 */
static bool extractStreamTitle(const char *ml, char *streamtitle, size_t bufsize) {
  const char *p1;
  const char *p2;
  size_t len;

  if (streamtitle == nullptr || bufsize == 0) {
    return false;
  }

  streamtitle[0] = '\0';
  p1 = findStreamTitleTag(ml);
  if (p1 == nullptr) {
    return false;
  }

  dbgprint("Streamtitle found, %d bytes", strlen(ml));
  dbgprint(ml);

  if (*p1 == '\'') {
    p1++;
    p2 = strchr(p1, '\'');
    if (p2 == nullptr) {
      return false;
    }

    len = (size_t)(p2 - p1);
    if (len >= bufsize) {
      len = bufsize - 1;
    }

    strncpy(streamtitle, p1, len);
    streamtitle[len] = '\0';
    return streamtitle[0] != '\0';
  }

  p2 = strchr(p1, ';');
  if (p2 != nullptr) {
    len = (size_t)(p2 - p1);
  } else {
    len = strlen(p1);
  }

  while (len > 0 && (*p1 == ' ' || *p1 == '\'')) {
    p1++;
    len--;
  }

  while (len > 0 && (p1[len - 1] == ' ' || p1[len - 1] == '\'')) {
    len--;
  }

  if (len == 0) {
    return false;
  }

  if (len >= bufsize) {
    len = bufsize - 1;
  }

  strncpy(streamtitle, p1, len);
  streamtitle[len] = '\0';
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
    p2 = p1 + 3;

#if defined(BLUETFT)
    (void)p2;
#else
    if (displaytype == T_NEXTION) {
      *p1++ = '\\';
      *p1++ = 'r';
    } else {
      *p1++ = '\n';
    }

    if (*p2 == ' ') {
      p2++;
    }
    strcpy(p1, p2);
#endif
  }
}

#if defined(BLUETFT)
#  include "display/dsp_dashboard.h"
#  include "net/wifi_display.h"
#endif

/**
 * @brief Show artist and song title if present in metadata.
 *        Show always if full=true.
 *
 * @param ml Metadata string or plain text
 * @param full If true, show even without StreamTitle tag
 */
void showStreamDataOnTFT(const char *ml, bool full) {
  char streamtitle[150];
#if !defined(BLUETFT)
  char displaytitle[150];
#endif

  if (extractStreamTitle(ml, streamtitle, sizeof(streamtitle))) {
    metaSetTitle(streamtitle);
    addTitleToHistory(streamtitle);
#if defined(BLUETFT)
    dspDashboardInvalidateTitle();
#else
    strncpy(displaytitle, streamtitle, sizeof(displaytitle) - 1);
    displaytitle[sizeof(displaytitle) - 1] = '\0';
    formatStreamTitle(displaytitle);
    showTextOnTFT(1, displaytitle);
#endif
  } else if (full && ml != nullptr && ml[0] != '\0') {
    strncpy(streamtitle, ml, sizeof(streamtitle));
    streamtitle[sizeof(streamtitle) - 1] = '\0';
    metaSetTitle(streamtitle);
    addTitleToHistory(streamtitle);
#if defined(BLUETFT)
    dspDashboardInvalidateTitle();
#else
    showTextOnTFT(1, streamtitle);
#endif
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
  String normalized = host;
  bool httpsScheme = false;

  normalized.trim();
  if (normalized.startsWith("http://")) {
    normalized = normalized.substring(7);
  } else if (normalized.startsWith("https://")) {
    normalized = normalized.substring(8);
    httpsScheme = true;
  }

  hostwoext = normalized;
  port = 80;
  extension = "/";

  // Extract extension
  inx = normalized.indexOf("/");
  if (inx >= 0) {
    extension = normalized.substring(inx);
    hostwoext = normalized.substring(0, inx);
  }

  // Extract port number
  inx = hostwoext.indexOf(":");
  if (inx >= 0) {
    uint16_t parsedPort = (uint16_t)hostwoext.substring(inx + 1).toInt();
    if (parsedPort > 0) {
      port = parsedPort;
    }
    hostwoext = hostwoext.substring(0, inx);
  } else if (httpsScheme) {
    // Firmware streams via plain HTTP client only; keep default HTTP port.
    port = 80;
  }

  if (extension.length() == 0) {
    extension = "/";
  }
}

/**
 * @brief Decode URL-encoded path parts (%xx and +) to plain text.
 *
 * @param text URL encoded text
 * @return String Decoded text
 */
static String decodeUrlText(const String &text) {
  String decoded = "";
  size_t i = 0;

  decoded.reserve(text.length());
  while (i < text.length()) {
    char c = text[i];
    if ((c == '%') && (i + 2 < text.length())) {
      char h1 = text[i + 1];
      char h2 = text[i + 2];
      if (isxdigit((unsigned char)h1) && isxdigit((unsigned char)h2)) {
        char tmp[3];
        char *endptr = nullptr;
        long value;
        tmp[0] = h1;
        tmp[1] = h2;
        tmp[2] = '\0';
        value = strtol(tmp, &endptr, 16);
        if (endptr != nullptr && *endptr == '\0') {
          decoded += (char)value;
          i += 3;
          continue;
        }
      }
    }
    if (c == '+') {
      decoded += ' ';
    } else {
      decoded += c;
    }
    i++;
  }

  return decoded;
}

/**
 * @brief Build a display title from an MP3 URL path.
 *
 * @param extension URL path including optional query
 * @return String Decoded filename without .mp3 extension
 */
static String extractMp3TitleFromExtension(const String &extension) {
  String path = extension;
  String filename = "";
  int inx;

  inx = path.indexOf("?");
  if (inx >= 0) {
    path = path.substring(0, inx);
  }

  inx = path.indexOf("#");
  if (inx >= 0) {
    path = path.substring(0, inx);
  }

  inx = path.lastIndexOf("/");
  if (inx >= 0) {
    filename = path.substring(inx + 1);
  } else {
    filename = path;
  }

  filename.trim();
  if (filename.length() < 5 || !filename.endsWith(".mp3")) {
    return String("");
  }

  filename.remove(filename.length() - 4); // Remove ".mp3"
  filename = decodeUrlText(filename);
  filename.trim();
  return filename;
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
  String mp3Title;

  stop_mp3client();
  dbgprint("Connect to new host %s", host.c_str());
  if (host.startsWith("https://")) {
    dbgprint("HTTPS URL detected; trying plain HTTP request to same host");
  }

  showTextOnTFT(0, "Webradio");
  renderTimeOnTFT("");

  datamode = INIT;
  chunked = false;

  // Handle M3U playlist
  if (host.endsWith(".m3u")) {
    handlePlaylistRequest();
  }

  // Parse host components
  parseHostString(host, hostwoext, port, extension);
  mp3Title = extractMp3TitleFromExtension(extension);
  if (mp3Title.length() > 0) {
    metaSetStation("MP3 Stream");
    showTextOnTFT(2, "MP3 Stream");
    showStreamDataOnTFT(mp3Title.c_str(), true);
  }

  dbgprint("Connect to %s on port %d, extension %s", hostwoext.c_str(), port, extension.c_str());
#if !defined(BLUETFT)
  {
    const char *s = dbgprint("Connect to %s on port %d, extension %s", hostwoext.c_str(), port, extension.c_str());
    showTextOnTFT(1, s);
  }
#endif

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
static bool streamStartWarmupPending = false;
static uint16_t streamStartWarmupChunks = 0;
static uint32_t streamStartWarmupMs = 0;
static const uint16_t STREAM_START_WARMUP_MIN_CHUNKS = 24;
static const uint32_t STREAM_START_WARMUP_MIN_MS = 180;
static bool streamFirstStartAfterBoot = true;
static const uint16_t STREAM_START_BOOT_WARMUP_MIN_CHUNKS = 72;
static const uint32_t STREAM_START_BOOT_WARMUP_MIN_MS = 650;

#include "stream_watchdog.h"

static void processQDataChunk() {
  while (!claimSPI("chunk")) {
    vTaskDelay(1);
  }
  vs1053player->playChunk(inchunk.buf, sizeof(inchunk.buf));
  releaseSPI();
  totalcount += sizeof(inchunk.buf);
  streamWatchdogFeed((uint16_t)sizeof(inchunk.buf));

  if (streamStartWarmupPending) {
    const bool bootWarmup = streamFirstStartAfterBoot;
    const uint16_t minChunks = bootWarmup ? STREAM_START_BOOT_WARMUP_MIN_CHUNKS : STREAM_START_WARMUP_MIN_CHUNKS;
    const uint32_t minMs = bootWarmup ? STREAM_START_BOOT_WARMUP_MIN_MS : STREAM_START_WARMUP_MIN_MS;
    streamStartWarmupChunks++;
    if ((streamStartWarmupChunks >= minChunks) &&
        (millis() - streamStartWarmupMs >= minMs)) {
      streamStartWarmupPending = false;
      streamFirstStartAfterBoot = false;
      beginStreamAudioOnDecoderStart();
    }
  }
}

/**
 * @brief Process QSTARTSONG chunk type - start the player.
 */
static void processQStartSong() {
  const bool reconnect = !streamFirstStartAfterBoot;

  streamStartWarmupPending = false;
  streamStartWarmupChunks = 0;
  streamStartWarmupMs = millis();

  while (!claimSPI("startsong")) {
    vTaskDelay(1);
  }
  if (!reconnect) {
    vs1053player->setOutputEnabled(false);
  }
  vs1053player->startSong();
  releaseSPI();

  if (reconnect) {
    beginStreamAudioOnDecoderStart();
    return;
  }

  streamStartWarmupPending = true;
}

/**
 * @brief Process QSTOPSONG chunk type - stop the player.
 */
static void processQStopSong() {
  streamStartWarmupPending = false;
  streamStartWarmupChunks = 0;
  if (!claimSPI("stopsong")) {
    return;
  }
  vs1053player->setVolume(0);
  vs1053player->stopSong();
  releaseSPI();
  vTaskDelay(50 / portTICK_PERIOD_MS);
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
 * @brief Handle TFT info display updates.
 */
static void handleTftInfoDisplay() {
  if (time_req) {
    time_req = false;
    if (NetworkFound) {
      renderTimeOnTFT(timetxt);
      renderVolumeOnTFT();
      renderBatteryChargeOnTFT();
    }
  }
}

/**
 * @brief Handle TFT update with SPI bus management.
 */
static void handleTftUpdate() {
  bool spiClaimed = false;
  static uint32_t lastLiveDashboardMs = 0;

  if (dsp_isUseSPI()) {
    spiClaimed = claimSPI("hspectft");
  }

  if (tft) {
    handle_tft_txt();
#if defined(BLUETFT)
    if (datamode == STOPPED || (millis() - lastLiveDashboardMs) >= 2000UL) {
      dspDashboardRefreshLiveStatus();
      updateWifiStatusDisplay();
      lastLiveDashboardMs = millis();
    }
#endif
    handleTftInfoDisplay();
    dsp_update();
  }

  if (spiClaimed) {
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

  if (!vs1053Available || !vs1053player) {
    return;
  }

  if (!claimSPI("hspec")) {
    return;
  }
  vs1053player->setVolume(getChipVolume()); // Map external 0-100 to audible 40-100

  if (reqtone) {
    reqtone = false;
    vs1053player->setTone(ini_block.rtone);
  }

  releaseSPI();
}

/**
 * @brief Handle special (non-stream data) functions for spftask.
 *        Runs once per spftask iteration.
 */
void handleRenderData() {
  // While a firmware update is running, the OTA screen (drawn from loop()) owns
  // the TFT and the SPI bus. Skip all rendering and VS1053 SPI access here so we
  // do not draw the dashboard over the OTA progress screen (which would cause
  // artifacts and overwrite the size/ETA lines) or corrupt the shared SPI bus.
  if (otaActive) {
    return;
  }

  // 1. Time and alarm (no SPI)
  handleTimeAndAlarmUpdate();

  // 2. VS1053 volume and tone
  handleVs1053VolumeAndTone();

  // 3. TFT dashboard + segment queue (SPI claimed inside)
  handleTftUpdate();
}