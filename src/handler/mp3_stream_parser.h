#ifndef MP3_STREAM_PARSER_H
#define MP3_STREAM_PARSER_H

/**
 * @brief Parses HTTP stream bytes (headers, metadata, playlists, MP3 data).
 */
class Mp3StreamParser {
public:
  void handleByte(uint8_t b) {

  // Test if DATA handling
  if (chunked && (datamode & (DATA | METADATA | PLAYLISTDATA))) {
    if (chunkcount == 0) { // If we are expecting a new chunkcount
      if (b == '\r') {     // Skip CR
        return;
      } else if (b == '\n') {   // If byte is NEWLINE
        chunkcount = chunkSizeHex_; // set new count
        chunkSizeHex_ = 0;          // For next decode
        return;
      }

      // We have received a hexadecimal character.  Decode it and add to the
      // result.
      b = toupper(b) - '0'; // Be sure we have uppercase
      if (b > 9) {
        b = b - 7; // Translate A..F to 10..15
      }

      chunkSizeHex_ = (chunkSizeHex_ << 4) + b;
      return;
    }
    chunkcount--; // Update count to next chunkSizeHex_ block
  }

  if (datamode == DATA) { // Handle next byte of MP3/Ogg data
    if (vs1053Available) {
      if (outqp >= (outchunk.buf + sizeof(outchunk.buf))) {
        if (xQueueSend(dataqueue, &outchunk, pdMS_TO_TICKS(100)) != pdTRUE) {
          return;
        }
        outqp = outchunk.buf;
      }
      *outqp++ = b;
    }

    if (metaint) {            // No METADATA on Ogg streams or mp3 files
      if (--datacount == 0) { // End of datablock
        datamode = METADATA;
        metadataLineIndex = -1; // Expecting first metabyte (counter)
      }
    }
    return;
  }

  if (datamode == INIT) { // Initialize for header receive
    contentTypeSeen_ = false;       // Contents type not seen yet
    metaint = 0;          // No metaint found
    headerLineFeedCount_ = 0;          // For detection end of header
    bitrate = 0;          // Bitrate still unknown
    mbitrate = 0;
    dbgprint("Switch to HEADER");
    datamode = HEADER; // Handle header
    totalcount = 0;    // Reset totalcount
    metadataLineIndex = 0;   // No metadata yet
    metalinebf[0] = '\0';
  }

  if (datamode == HEADER) { // Handle next byte of MP3 header
    if ((b > 0x7F) ||       // Ignore unprintable characters
        (b == '\r') ||      // Ignore CR
        (b == '\0'))        // Ignore NULL
    {
      // NOTHING TO DO
    } else if (b == '\n') {                     // Linefeed
      headerLineFeedCount_++;                                // Count linefeeds
      metalinebf[metadataLineIndex] = '\0';           // Take care of delimiter
      if (checkHeaderLine(metalinebf)) {        // if there is a reasonable input
        dbgprint("Headerline: %s", metalinebf); // Show headerline
        String metaline = String(metalinebf);   // Convert to string
        String lcml = metaline;                 // Use lower case for compare
        lcml.toLowerCase();

        if (lcml.startsWith("location: http://")) { // On redirection get the new URL
          host = metaline.substring(17);            // get new URL
          hostreq = true;                           // And request this one
        } else if (lcml.startsWith("location: https://")) {
          host = metaline.substring(18);
          hostreq = true;
        }

        if (lcml.indexOf("content-type") >= 0) { // Line with "Content-Type: xxxx/yyy"
          contentTypeSeen_ = true;                         // remember seeing this
          String ct = metaline.substring(13);    // Set contentstype. Not used yet
          ct.trim();
          dbgprint("%s seen.", ct.c_str());
        }

        if (lcml.startsWith("icy-br:")) {
          bitrate = metaline.substring(7).toInt(); // Found bitrate tag, read the bitrate
          if (bitrate > 1000) {                    // Often bits/sec (e.g. 128000)
            bitrate = (bitrate + 500) / 1000;
          }
          if (bitrate == 0) {                      // For Ogg br is like "Quality 2"
            bitrate = 87;                          // Dummy bitrate
          }
        } else if (lcml.startsWith("icy-metaint:")) {
          metaint = metaline.substring(12).toInt(); // Found metaint tag, read the value
        } else if (lcml.startsWith("icy-name:")) {
          String stationName = metaline.substring(9); // Get station name
          stationName.trim();                          // Remove leading and trailing spaces
          metaSetStation(stationName.c_str());         // Publish thread-safe
          showTextOnTFT(2, stationName);               // Set screen segment bottom part
        } else if (lcml.startsWith("transfer-encoding:")) {
          // Station provides chunked transfer
          if (lcml.endsWith("chunked")) {
            chunked = true; // Remember chunked transfer mode
            chunkcount = 0; // Expect chunkcount in DATA
          }
        }
      }

      metadataLineIndex = 0;                // Reset this line
      if ((headerLineFeedCount_ == 2) && contentTypeSeen_) { // If Content type seen and a double LF available
        dbgprint("Switch to DATA, bitrate is %d"
                 ", metaint is %d",
                 bitrate, metaint);           // Show bitrate and metaint
        datamode = DATA;                      // Expecting data now
        datacount = metaint;                  // Number of bytes before first metadata
        queueFunctionForPlayTask(QSTARTSONG); // Queue a request to start song
      }
    } else {
      metalinebf[metadataLineIndex++] = (char)b; // Normal character, put new char in metaline
      if (metadataLineIndex >= METASIZ) {        // Prevent overflow
        metadataLineIndex--;
      }
      headerLineFeedCount_ = 0; // Reset double CRLF detection
    }
    return;
  }

  if (datamode == METADATA) { // Handle next byte of metadata
    if (metadataLineIndex < 0) {    // If first byte of metadata
      metadataLineIndex = 0;        // Prepare to store first character
      metacount = b * 16 + 1; // New count for metadata including length byte
      if (metacount > 1) {
        dbgprint("Metadata block %d bytes",
                 metacount - 1); // Most of the time there are zero bytes of metadata
      }
    } else {
      if (metadataLineIndex < (METASIZ - 1)) {
        metalinebf[metadataLineIndex++] = (char)b;
      }
    }

    if (--metacount == 0) {
      metalinebf[metadataLineIndex] = '\0'; // Make sure line is limited
      if (strlen(metalinebf)) {       // If any info available
        // metaline contains artist and song name.  For example:
        // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
        // Sometimes it is just other info like:
        // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
        // Isolate the StreamTitle, remove leading and trailing quotes if
        // present.
        showStreamDataOnTFT(metalinebf); // Show artist and title if present in metadata
      }

      if (metadataLineIndex >= (METASIZ - 1)) {
        dbgprint("Metadata block truncated to %d bytes", METASIZ - 1);
      }

      datacount = metaint; // Reset data count
      // bufcnt = 0;                                     // Reset buffer count
      datamode = DATA; // Expecting data
    }
  }

  if (datamode == PLAYLISTINIT) { // Initialize for receive .m3u file
    // We are going to use metadata to read the lines from the .m3u file
    // Sometimes this will only contain a single line
    metadataLineIndex = 0;           // Prepare for new line
    headerLineFeedCount_ = 0;               // For detection end of header
    datamode = PLAYLISTHEADER; // Handle playlist data
    playlistEntryIndex_ = 1;           // Reset for compare
    totalcount = 0;            // Reset totalcount
    clength = 0xFFFFFFFF;      // Content-length unknown
    playlistHtmlResponse_ = false;
    playlistRedirectPending_ = false;
    playlistHttpsRedirect_ = false;
    dbgprint("Read from playlist");
  }

  if (datamode == PLAYLISTHEADER) { // Read header
    if ((b > 0x7F) ||               // Ignore unprintable characters
        (b == '\r') ||              // Ignore CR
        (b == '\0'))                // Ignore NULL
    {
      return;                                     // Quick return
    } else if (b == '\n') {                       // If Linefeed
      String metaline;
      String lcml;

      headerLineFeedCount_++;                                  // Count linefeeds
      metalinebf[metadataLineIndex] = '\0';             // Take care of delimeter
      dbgprint("Playlistheader: %s", metalinebf); // Show playlistheader
      scan_content_length(metalinebf);            // Check if it is a content-length line

      metaline = String(metalinebf);
      lcml = metaline;
      lcml.toLowerCase();

      if (lcml.startsWith("http/1.") &&
          ((lcml.indexOf(" 301 ") >= 0) || (lcml.indexOf(" 302 ") >= 0) || (lcml.indexOf(" 303 ") >= 0))) {
        playlistRedirectPending_ = true;
      }

      if ((lcml.indexOf("content-type") >= 0) && (lcml.indexOf("text/html") >= 0)) {
        playlistHtmlResponse_ = true;
      }

      if (lcml.startsWith("location:")) {
        String location = metaline.substring(9);
        location.trim();
        if (location.startsWith("https://")) {
          playlistHttpsRedirect_ = true;
        } else if (applyHttpLocationHeader(location)) {
          playlistRedirectPending_ = true;
        }
      }

      if (lcml.startsWith("transfer-encoding:") && (lcml.indexOf("chunked") >= 0)) {
        chunked = true;
        chunkcount = 0;
      }

      metadataLineIndex = 0;                            // Ready for next line
      if (headerLineFeedCount_ == 2) {
        if (playlistHttpsRedirect_) {
          dbgprint("Playlist HTTPS redirect not supported, aborting");
          playlistHttpsRedirect_ = false;
          playlistRedirectPending_ = false;
          playlistHtmlResponse_ = false;
          playlist_num = 0;
          datamode = STOPPED;
          return;
        }

        if (playlistRedirectPending_) {
          dbgprint("Playlist redirect to %s", host.c_str());
          playlistRedirectPending_ = false;
          playlistHtmlResponse_ = false;
          datamode = STOPPED;
          hostreq = true;
          return;
        }

        if (playlistHtmlResponse_) {
          dbgprint("Playlist response is HTML, aborting");
          playlist_num = 0;
          playlistHtmlResponse_ = false;
          datamode = STOPPED;
          return;
        }

        dbgprint("Switch to PLAYLISTDATA, search for entry %d", playlist_num);
        datamode = PLAYLISTDATA; // Expecting data now
        return;
      }
    } else {
      metalinebf[metadataLineIndex++] = (char)b; // Normal character, put new char in metaline
      if (metadataLineIndex >= METASIZ) {        // Prevent overflow
        metadataLineIndex--;
      }
      headerLineFeedCount_ = 0; // Reset double CRLF detection
    }
  }

  if (datamode == PLAYLISTDATA) { // Read next byte of .m3u file data
    clength--;                    // Decrease content length by 1
    if ((b > 0x7F) ||             // Ignore unprintable characters
        (b == '\r') ||            // Ignore CR
        (b == '\0'))              // Ignore NULL
    {
      // ignore
    }

    if (b != '\n') {                       // If Linefeed
                                           // No, normal character in playlistdata,
      metalinebf[metadataLineIndex++] = (char)b; // add it to metaline
      if (metadataLineIndex >= METASIZ) {        // Prevent overflow
        metadataLineIndex--;
      }
    }

    if ((b == '\n') ||  // If linefeed
        (clength == 0)) // Or end of playlist data contents
    {
      int inx;                        // Pointer in metaline
      metalinebf[metadataLineIndex] = '\0'; // Take care of delimeter
      dbgprint("Playlistdata: %s",    // Show playlistheader
               metalinebf);
      if (strlen(metalinebf) < 5) { // Skip short lines
        metadataLineIndex = 0;            // Flush line
        metalinebf[0] = '\0';
        return;
      }

      String metaline = String(metalinebf);    // Convert to string
      if (metaline.indexOf("#EXTINF:") >= 0) { // If Info
        if (playlist_num == playlistEntryIndex_) {     // If Info for this entry
          inx = metaline.indexOf(",");         // Comma in this line?
          if (inx > 0) {
            // Show artist and title if present in metadata
            showStreamDataOnTFT(metaline.substring(inx + 1).c_str(), true);
          }
        }
      }

      if (metaline.startsWith("#")) { // // If Commentline
        metadataLineIndex = 0;              // ignore
        return;                       // Ignore commentlines
      }

      if (!isValidPlaylistEntry(metalinebf)) {
        dbgprint("Skipping invalid playlist line: %s", metalinebf);
        metadataLineIndex = 0;
        metalinebf[0] = '\0';
        return;
      }

      // Now we have an URL for a .mp3 file or stream.  Is it the rigth one?
      dbgprint("Entry %d in playlist found: %s", playlistEntryIndex_, metalinebf);
      if (playlist_num == playlistEntryIndex_) {
        inx = metaline.indexOf("http://");    // Search for "http://"
        if (inx >= 0) {                       // Does URL contain "http://"?
          host = metaline.substring(inx + 7); // remove it and set host
        } else if (metaline.startsWith("https://")) {
          host = metaline.substring(8);
        } else {
          host = metaline; // set new host
        }
        if (!connectToWebradioServer()) {
          dbgprint("Playlist stream connection failed, aborting");
          playlist_num = 0;
          datamode = STOPPED;
        }
      }

      metadataLineIndex = 0; // Prepare for next line
      host = playlist; // Back to the .m3u host
      playlistEntryIndex_++;   // Next entry in playlist
    }
  }
}

/**
 * @desc	Called from the main loop() for the mp3 functions.
 * 			A connection to an MP3 server is active and we are ready
 * to receive data. Normally there is about 2 to 4 kB available in the data
 * stream.  This depends on the sender.
 * */
void mp3loop() {
  uint32_t maxchunk; // Max number of bytes to read
  int res = 0;       // Result reading from mp3 stream
  uint32_t av = 0;   // Available in stream
  String nodeID;     // Next nodeID of track on SD
  uint32_t timing;   // Startime and duration this function
  uint32_t qspace;   // Free space in data queue

  // Try to keep the Queue to playStreamFromQueue filled up by adding as much
  // bytes as possible Check if playing
  if (datamode & (INIT | HEADER | DATA | METADATA | PLAYLISTINIT | PLAYLISTHEADER | PLAYLISTDATA)) {
    timing = millis();                           // Start time this function
    maxchunk = sizeof(tmpbuff);                  // Reduce byte count for this mp3loop()
    if (vs1053Available) {
      qspace = uxQueueSpacesAvailable(dataqueue) * // Compute free space in data queue
               sizeof(qdata_struct);
    } else {
      qspace = maxchunk;
    }

    if (localfile) {      // If playing file from SD card
      av = mp3filelength; // Bytes left in file

      // Reduce byte count for this mp3loop()
      if (av < maxchunk) {
        maxchunk = av;
      }

      if (maxchunk > qspace) { // Check if there is enough space in queue
        maxchunk = qspace;     // No, limit to free queue space
      }

      if (maxchunk) {                          // Check if there is anything to read
        if (!claimSPI("sdread")) {
          return;
        }
        res = mp3file.read(tmpbuff, maxchunk); // Read a block of data
        releaseSPI();                          // Release SPI bus
        mp3filelength -= res;                  // Number of bytes left
      }
    } else {                      // Play next
      av = mp3client.available(); // Available from stream

      if (av < maxchunk) { // Limit read size
        maxchunk = av;
      }

      if (maxchunk > qspace) { // Check if there is enough space in queue
        maxchunk = qspace;     // No, limit to free queue space
      }

      // Check if there is anything to read
      if (maxchunk) {
        res = mp3client.read(tmpbuff, maxchunk); // Read a number of bytes from the stream
        streamWatchdogFeed((uint16_t)res);
        vuMeterFeed((uint16_t)res);
      } else {
        if (datamode == PLAYLISTDATA) { // If End of playlist
          playlist_num = 0;             // And reset
          dbgprint("End of playlist seen");
          datamode = STOPPED;
          ini_block.newpreset++; // Go to next preset
        }
      }
    }

    for (int i = 0; i < res; i++) {
      handleNextDataByteFromServer(tmpbuff[i]); // Handle one byte
    }

    timing = millis() - timing;                // Duration this function
    if (timing > max_mp3loop_time) {           // New maximum found
      max_mp3loop_time = timing;               // set new maximum
      dbgprint("Duration mp3loop %d", timing); // and report it
    }
  }

  if (datamode == STOPREQD) { // If STOP requested
    dbgprint("STOP requested");
    if (localfile) {
      if (claimSPI("close")) {
        mp3file.close();
        releaseSPI();
      }
    } else {
      stop_mp3client(); // Disconnect if still connected
    }

    chunked = false;                     // Not longer chunked
    datacount = 0;                       // Reset datacount
    outqp = outchunk.buf;                // and pointer
    queueFunctionForPlayTask(QSTOPSONG); // Queue a request to stop the song
    metaint = 0;                         // No metaint known now
    datamode = STOPPED;                  // state becomes STOPPED
    mbitrate = 0;
    bitrate = 0;
    return;
  }

  if (localfile) {         // If playing from SD-Card
    if (datamode & DATA) { // Test if playing
                           // End of mp3 data
      if (av == 0) {
        datamode = STOPREQD; // End of local mp3-file detected
        nodeID = selectnextSDnode(SD_currentnode,
                                  +1); // Select the next file on SD
        host = getSDfilename(nodeID);
        hostreq = true; // Request this host
      }
    }
  }

  if (ini_block.newpreset != currentpreset) { // If New station or next from playlist requested
    if (datamode != STOPPED) {                // If still busy stop the request
      datamode = STOPREQD;
    } else {
      if (playlist_num) { // If playing from playlist

        // retrieve URL of playlist
        playlist_num += ini_block.newpreset - currentpreset; // Next entry in playlist
        ini_block.newpreset = currentpreset;                 // Stay at current preset
      } else {
        host = nvsGetHostFromPref(); // Lookup preset in preferences
        chomp(host);                 // Get rid of part after "#"
      }

      dbgprint("New preset/file requested (%d/%d) from %s", ini_block.newpreset, playlist_num, host.c_str());

      if (host != "") { // if Preset in ini-file
        hostreq = true; // Force this station as new preset
      } else {
        // This preset is not available, return to preset 0, will be handled in
        // next mp3loop()
        dbgprint("No host for this preset");
        ini_block.newpreset = 0; // Wrap to first station
      }
    }
  }

  if (hostreq) { // New preset or station
    hostreq = false;
    currentpreset = ini_block.newpreset; // Remember current preset
    // Find out if this URL is on localhost (SD).
    localfile = (host.indexOf("localhost/") >= 0);
    if (localfile) {         // If playing file from localhost
      if (connecttofile()) { // open mp3-file
        datamode = DATA;     // Start in DATA mode
      }
    } else {
      if (host.startsWith("ihr/")) { // If iHeartRadio station requested
        host = host.substring(4);    // remove "ihr/"
        host = xmlgethost(host);     // Parse the xml to get the host
      }

      connectToWebradioServer(); // Switch to new host
    }
  }
  }

private:
  int chunkSizeHex_ = 0;
  uint16_t playlistEntryIndex_ = 0;
  int headerLineFeedCount_ = 0;
  bool contentTypeSeen_ = false;
  bool playlistHtmlResponse_ = false;
  bool playlistRedirectPending_ = false;
  bool playlistHttpsRedirect_ = false;
};

static Mp3StreamParser mp3StreamParser;

inline void handleNextDataByteFromServer(uint8_t b) {
  mp3StreamParser.handleByte(b);
}

#endif
