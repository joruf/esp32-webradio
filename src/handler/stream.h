/**
 * @desc  Show artist and songtitle if present in metadata
 * Show always if full=true
 */
void showStreamDataOnTFT(const char *ml, bool full) {
  char* p1 ;
  char* p2 ;
  char              streamtitle[150] ;           // Streamtitle from metadata

  if(strstr(ml, "StreamTitle=")) {
    dbgprint("Streamtitle found, %d bytes", strlen(ml));
    dbgprint(ml);

    // // Search for end of title
    p1 = (char*)ml + 12 ;                       // Begin of artist and title
    if(( p2 = strstr(ml, ";"))) {
      if(*p1 == '\'') { // Surrounded by quotes
        p1++ ;
        p2-- ;
      }
      *p2 = '\0' ;                              // Strip the rest of the line
    }

    // Save last part of string as streamtitle.  Protect against buffer overflow
    strncpy(streamtitle, p1, sizeof(streamtitle));
    streamtitle[sizeof(streamtitle)- 1] = '\0' ;
  } else if(full) { // Info probably from playlist
    strncpy(streamtitle, ml, sizeof(streamtitle));
    streamtitle[sizeof(streamtitle)- 1] = '\0' ;
  } else {
    icystreamtitle = "" ;                       // Unknown type
    return ;                                    // Do not show
  }

  // Save for status request from browser
  icystreamtitle = streamtitle ;

  // look for artist/title separator
  if(( p1 = strstr(streamtitle, " - "))) {
    p2 = p1 + 3 ;                               // 2nd part of text at this position
    if(displaytype == T_NEXTION) {
      *p1++ = '\\' ;                            // Found: replace 3 characters by "\r"
      *p1++ = 'r' ;                             // Found: replace 3 characters by "\r"
    } else {
      *p1++ = '\n' ;                            // Found: replace 3 characters by newline
    }

    // Leading space in title
    if(*p2 == ' ') {
      p2++;
    }
    strcpy(p1, p2);                         // Shift 2nd part of title 2 or 3 places
  }

  showTextOnTFT(1, streamtitle);                   // Set screen segment text middle part
}




/**
 * @desc  Connect to the Internet radio server specified by newpreset
 * */
bool connectToWebradioServer() {
  // ═══════════════════════════════════════════════════════════════
  // NEU: ABFANGSCHUTZ FÜR HTML-ZEICHENFOLGEN & HTTPS
  // ═══════════════════════════════════════════════════════════════
  if (host.startsWith("<") || host.indexOf("<html>") >= 0) {
    dbgprint("❌ Abbr: Ungültiger Hostname (HTML-Daten empfangen)");
    showTextOnTFT(1, "Error: Invalid Stream");
    return false;
  }

  if (host.startsWith("https://")) {
    dbgprint("❌ Abbr: HTTPS (SSL) wird nicht unterstützt: %s", host.c_str());
    showTextOnTFT(1, "Error: HTTPS not supported");
    return false;
  }
  // ═══════════════════════════════════════════════════════════════

  int         inx ;                                 // Position of ":" in hostname
  uint16_t    port = 80 ;                           // Port number for host
  String      extension = "/" ;                     // May be like "/mp3" in "skonto.ls.lv:8002/mp3"
  String      hostwoext = host ;                    // Host without extension and portnumber
  String      auth  ;                               // For basic authentication

  stop_mp3client();                                // Disconnect if still connected
  dbgprint("Connect to new host %s", host.c_str());

  showTextOnTFT(0, "Webradio");                     // Set screen segment text top line
  showTimeOnTFT("");                              // Clear time on TFT screen

  datamode = INIT ;                                 // Start default in metamode
  chunked = false ;                                 // Assume not chunked

  // m3u playlist
  if(host.endsWith(".m3u")) {
    playlist = host ;                               // Save copy of playlist URL
    datamode = PLAYLISTINIT ;                       // start in PLAYLIST mode

    // If first entry to play
    if(playlist_num == 0) {
      playlist_num = 1 ;                            // set index
    }
    dbgprint("Playlist request, entry %d", playlist_num);
  }

  // In the URL there may be an extension, like noisefm.ru:8000/play.m3u&t=.m3u
  inx = host.indexOf("/");                      // Search for begin of extension
  if(inx > 0) {
    extension = host.substring(inx);            // change the default
    hostwoext = host.substring(0, inx);         // Host without extension
  }

  // In the host there may be a portnumber
  inx = hostwoext.indexOf(":");                 // Search for separator
  if(inx >= 0) {
    port = host.substring(inx + 1 ).toInt();     // Get portnumber as integer
    hostwoext = host.substring(0, inx);         // Host without portnumber
  }

  const char* s;
  s = dbgprint("Connect to %s on port %d, extension %s", hostwoext.c_str(), port, extension.c_str());
  showTextOnTFT(1, s);

  if(mp3client.connect(hostwoext.c_str(), port)) {
    dbgprint("Connected to server");
    auth = nvsGetValue("basicauth");              // Use basic authentication
    if(auth != "") {
       auth = base64::encode(auth.c_str());     // Encode
       auth = String("Authorization: Basic ") + auth + String("\r\n");
    }

    mp3client.print(String("GET ")+
                      extension +
                      String(" HTTP/1.1\r\n")+
                      String("Host: ")+
                      hostwoext +
                      String("\r\n")+
                      String("Icy-MetaData:1\r\n")+
                      auth +
                      String("Connection: close\r\n\r\n"));
    return true ;
  }

  dbgprint("Request %s failed!", host.c_str());
  return false;
}



/**
 * @desc  Play stream data from input queue
 * Handle all I/O to VS1053B during normal playing
 * Handles display of text, time and volume on TFT as well
 * */
void playStreamFromQueue(void * parameter) {
  while(true) {
    if(xQueueReceive(dataqueue, &inchunk, 5)) {

      // If FIFO is full
      while(!vs1053player->data_request()) {
        vTaskDelay(1);                                          // take a break
      }

      // Select the chunk type
      switch(inchunk.datatyp) {
        case QDATA:
          claimSPI("chunk");                                    // Claim SPI bus
          vs1053player->playChunk(inchunk.buf,                    // DATA, send to player
                                    sizeof(inchunk.buf));
          releaseSPI();                                            // Release SPI bus
          totalcount += sizeof(inchunk.buf);                       // Count the bytes
          break ;
        case QSTARTSONG:
          claimSPI("startsong");                                // Claim SPI bus
          vs1053player->startSong();                               // START, start player
          releaseSPI();                                            // Release SPI bus
          break ;
        case QSTOPSONG:
          claimSPI("stopsong");                                 // Claim SPI bus
          vs1053player->setVolume(0);                           // Mute
          vs1053player->stopSong();                                // STOP, stop player
          releaseSPI();                                            // Release SPI bus
          vTaskDelay(500 / portTICK_PERIOD_MS);                 // Pause for a short time
          break ;
        default:
          break ;
      }
    }

    //esp_task_wdt_reset();                                        // Protect against idle cpu
  }

  //vTaskDelete(NULL);                                          // Will never arrive here
}


/**
 * @desc  Handle special (non-stream data) functions for spftask
 * runs once
 * */
void handleDisplayData()
{
  // 1. TFT Update-Block (Nutzt ID hspectft) mit Schutz vor Dauerfeuer
  bool updated = false;
  if(dsp_isUseSPI()) {
    claimSPI("hspectft"); // claim SPI bus
  }

  if(tft) {
    updated = handle_tft_txt(); // TFT refresh necessary
    if (updated) {
      dsp_update();             // Be sure to paint physical screen
    }
  }

  if(dsp_isUseSPI()) {
    releaseSPI();     // release SPI bus
  }

  // Wenn gezeichnet wurde, geben wir der CPU/MP3-Queue kurz Zeit (10ms) zum Atmen!
  if (updated) {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  // 2. Zeit- und Alarm-Berechnung (Kein SPI erforderlich!)
  if(time_req && NetworkFound) {
    getCurrentTime();
    handleRadioAlarm(); 
  }
  
  // 3. VS1053 Lautstärke- und Ton-Block (Extrem kurz halten!)
  static uint8_t last_vol = 255; 
  static bool last_sound_state = false;
  
  bool current_sound_state = isSoundPlaying();
  uint8_t target_vol = (current_sound_state == false) ? 0 : ini_block.reqvol;

  if (target_vol != last_vol || current_sound_state != last_sound_state || reqtone) {
    claimSPI("hspec"); 
    vs1053player->setVolume(target_vol);
    if(reqtone) {
      reqtone = false;
      vs1053player->setTone(ini_block.rtone); 
    }
    releaseSPI(); 
    last_vol = target_vol;
    last_sound_state = current_sound_state;
  }

  // 4. TFT Update-Anforderungen setzen
  if(time_req) {
    time_req = false; // clear request
    if(NetworkFound) {
      showTimeOnTFT(timetxt);
      displayVolumeOnTFT();
      displayBatteryChargeOnTFT();
    }
  }
}