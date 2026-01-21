


/**
 * @desc	Queue a special function for the play task.
 * */
void queueFunctionForPlayTask (int func) {
  qdata_struct     specchunk;                          // Special function to queue

  specchunk.datatyp = func;                            // Put function in datatyp
  xQueueSend (dataqueue, &specchunk, 200);           // Send to queue
}



/**
 * @desc	Check if a line in the header is a reasonable headerline.
 * 			Normally it should contain something like "icy-xxxx:abcdef".
 * */
bool checkHeaderLine (const char* str) {
  char    b;                                         // Byte examined
  int     len = 0;                                   // Lengte van de string

  while (( b = *str++)) { // Search to end of string
    len++;                                           // Update string length
    if (! isalpha (b)) { // Alpha (a-z, A-Z)
      if (b != '-') { // Minus sign is allowed
        if (b == ':') { // Found a colon
          return (( len > 5) && (len < 50));    // if length is okay
        } else {
          return false;                              // Not a legal character
        }
      }
    }
  }

  return false;                                      // End of string without colon
}



/**
 * @desc	Handle the next byte of data from server.
 * 			Chunked transfer encoding aware. Chunk extensions are not supported.
 * */
void handleNextDataByteFromServer (uint8_t b) {
  static int       chunksize = 0;                      // Chunkcount read from stream
  static uint16_t  playlistcnt;                        // Counter to find right entry in playlist
  static int       LFcount;                            // Detection of end of header
  static bool      ctseen = false;                     // First line of header seen or not

  // Test if DATA handling
  if (chunked && (datamode & (DATA | METADATA | PLAYLISTDATA))) {
    if (chunkcount == 0) { // If we are expecting a new chunkcount
      if (b == '\r') { // Skip CR
        return;
      } else if (b == '\n') { // If byte is NEWLINE
        chunkcount = chunksize;                       // set new count
        chunksize = 0;                                // For next decode
        return;
      }

      // We have received a hexadecimal character.  Decode it and add to the result.
      b = toupper (b) - '0';                        // Be sure we have uppercase
      if (b > 9) {
        b = b - 7;                                    // Translate A..F to 10..15
      }

      chunksize = (chunksize << 4) + b;
      return ;
    }
    chunkcount--;                                     // Update count to next chunksize block
  }

  if (datamode == DATA) { // Handle next byte of MP3/Ogg data
    *outqp++ = b;
    if (outqp == (outchunk.buf + sizeof(outchunk.buf))) { // If the buffer is full
      // Send data to playStreamFromQueue queue.  If the buffer cannot be placed within 200 ticks,
      // the queue is full, while the sender tries to send more.  The chunk will be discarded it that case.
      xQueueSend (dataqueue, &outchunk, 200);       // Send to queue
      outqp = outchunk.buf;                           // Item empty now
    }

    if (metaint) { // No METADATA on Ogg streams or mp3 files
      if (--datacount == 0) { // End of datablock
        datamode = METADATA;
        metalinebfx = -1;                             // Expecting first metabyte (counter)
      }
    }
    return;
  }

  if (datamode == INIT) { // Initialize for header receive
    ctseen = false;                                   // Contents type not seen yet
    metaint = 0;                                      // No metaint found
    LFcount = 0;                                      // For detection end of header
    bitrate = 0;                                      // Bitrate still unknown
    dbgprint ("Switch to HEADER");
    datamode = HEADER;                                // Handle header
    totalcount = 0;                                   // Reset totalcount
    metalinebfx = 0;                                  // No metadata yet
    metalinebf[0] = '\0';
  }

  if (datamode == HEADER) { // Handle next byte of MP3 header
    if ((b > 0x7F) ||                               // Ignore unprintable characters
         (b == '\r') ||                              // Ignore CR
         (b == '\0'))                               // Ignore NULL
    {
      // NOTHING TO DO
    } else if (b == '\n') { // Linefeed
      LFcount++;                                      // Count linefeeds
      metalinebf[metalinebfx] = '\0';                 // Take care of delimiter
      if (checkHeaderLine (metalinebf)) { // if there is a reasonable input
        dbgprint ("Headerline: %s", metalinebf); // Show headerline
        String metaline = String (metalinebf);      // Convert to string
        String lcml = metaline;                       // Use lower case for compare
        lcml.toLowerCase();

        if (lcml.startsWith ("location: http://")) { // On redirection get the new URL
          host = metaline.substring (17);           // get new URL
          hostreq = true;                             // And request this one
        }

        if (lcml.indexOf ("content-type") >= 0) { // Line with "Content-Type: xxxx/yyy"
          ctseen = true;                              // remember seeing this
          String ct = metaline.substring (13);      // Set contentstype. Not used yet
          ct.trim();
          dbgprint ("%s seen.", ct.c_str());
        }

        if (lcml.startsWith ("icy-br:")) {
          bitrate = metaline.substring(7).toInt();    // Found bitrate tag, read the bitrate
          if (bitrate == 0) { // For Ogg br is like "Quality 2"
            bitrate = 87;                             // Dummy bitrate
          }
        } else if (lcml.startsWith ("icy-metaint:")) {
          metaint = metaline.substring(12).toInt();   // Found metaint tag, read the value
        } else if (lcml.startsWith ("icy-name:")) {
          icyname = metaline.substring(9);            // Get station name
          icyname.trim();                             // Remove leading and trailing spaces
          showTextOnTFT (2, icyname);                      // Set screen segment bottom part
        } else if (lcml.startsWith ("transfer-encoding:")) {
          // Station provides chunked transfer
          if (lcml.endsWith ("chunked")) {
            chunked = true;                           // Remember chunked transfer mode
            chunkcount = 0;                           // Expect chunkcount in DATA
          }
        }
      }

      metalinebfx = 0;                                // Reset this line
      if (( LFcount == 2) && ctseen) { // If Content type seen and a double LF available
        dbgprint ("Switch to DATA, bitrate is %d"", metaint is %d", bitrate, metaint); // Show bitrate and metaint
        datamode = DATA;                              // Expecting data now
        datacount = metaint;                          // Number of bytes before first metadata
        queueFunctionForPlayTask (QSTARTSONG);                     // Queue a request to start song
      }
    } else {
      metalinebf[metalinebfx++] = (char)b;            // Normal character, put new char in metaline
      if (metalinebfx >= METASIZ) { // Prevent overflow
        metalinebfx--;
      }
      LFcount = 0;                                    // Reset double CRLF detection
    }
    return;
  }

  if (datamode == METADATA) { // Handle next byte of metadata
    if (metalinebfx < 0) { // If first byte of metadata
      metalinebfx = 0;                                // Prepare to store first character
      metacount = b * 16 + 1;                         // New count for metadata including length byte
      if (metacount > 1) {
        dbgprint ("Metadata block %d bytes",
                   metacount - 1);                   // Most of the time there are zero bytes of metadata
      }
    } else {
      metalinebf[metalinebfx++] = (char)b;            // Normal character, put new char in metaline
      if (metalinebfx >= METASIZ) { // Prevent overflow
        metalinebfx--;
      }
    }

    if (--metacount == 0) {
      metalinebf[metalinebfx] = '\0';                 // Make sure line is limited
      if (strlen (metalinebf)) { // If any info available
        // metaline contains artist and song name.  For example:
        // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
        // Sometimes it is just other info like:
        // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
        // Isolate the StreamTitle, remove leading and trailing quotes if present.
        showStreamDataOnTFT (metalinebf);               // Show artist and title if present in metadata
      }

      if (metalinebfx  > (METASIZ - 10)) { // If Unlikely metaline length
        dbgprint ("Metadata block too long! Skipping all Metadata from now on.");
        metaint = 0;                                  // Probably no metadata
      }

      datacount = metaint;                            // Reset data count
      //bufcnt = 0;                                     // Reset buffer count
      datamode = DATA;                                // Expecting data
    }
  }

  if (datamode == PLAYLISTINIT) { // Initialize for receive .m3u file
    // We are going to use metadata to read the lines from the .m3u file
    // Sometimes this will only contain a single line
    metalinebfx = 0;                                  // Prepare for new line
    LFcount = 0;                                      // For detection end of header
    datamode = PLAYLISTHEADER;                        // Handle playlist data
    playlistcnt = 1;                                  // Reset for compare
    totalcount = 0;                                   // Reset totalcount
    clength = 0xFFFFFFFF;                             // Content-length unknown
    dbgprint ("Read from playlist");
  }

  if (datamode == PLAYLISTHEADER) { // Read header
    if (( b > 0x7F) ||                               // Ignore unprintable characters
         (b == '\r') ||                              // Ignore CR
         (b == '\0'))                               // Ignore NULL
    {
      return;                                         // Quick return
    } else if (b == '\n') { // If Linefeed
      LFcount++;                                      // Count linefeeds
      metalinebf[metalinebfx] = '\0';                 // Take care of delimeter
      dbgprint ("Playlistheader: %s", metalinebf); // Show playlistheader
      scan_content_length (metalinebf);             // Check if it is a content-length line
      metalinebfx = 0;                                // Ready for next line
      if (LFcount == 2) {
        dbgprint ("Switch to PLAYLISTDATA, search for entry %d", playlist_num);
        datamode = PLAYLISTDATA;                      // Expecting data now
        return;
      }
    } else {
      metalinebf[metalinebfx++] = (char)b;            // Normal character, put new char in metaline
      if (metalinebfx >= METASIZ) { // Prevent overflow
        metalinebfx--;
      }
      LFcount = 0;                                    // Reset double CRLF detection
    }
  }

  if (datamode == PLAYLISTDATA) { // Read next byte of .m3u file data
    clength--;                                        // Decrease content length by 1
    if (( b > 0x7F) ||                               // Ignore unprintable characters
         (b == '\r') ||                              // Ignore CR
         (b == '\0'))                               // Ignore NULL
    {
      // ignore
    }

    if (b != '\n') { // If Linefeed
    	// No, normal character in playlistdata,
      metalinebf[metalinebfx++] = (char)b;            // add it to metaline
      if (metalinebfx >= METASIZ) { // Prevent overflow
        metalinebfx--;
      }
    }

    if (( b == '\n') ||                              // If linefeed
         (clength == 0))                            // Or end of playlist data contents
    {
      int inx;                                        // Pointer in metaline
      metalinebf[metalinebfx] = '\0';                 // Take care of delimeter
      dbgprint ("Playlistdata: %s",                   // Show playlistheader
                 metalinebf);
      if (strlen (metalinebf) < 5) { // Skip short lines
        metalinebfx = 0;                              // Flush line
        metalinebf[0] = '\0';
        return;
      }

      String metaline = String (metalinebf);        // Convert to string
      if (metaline.indexOf ("#EXTINF:") >= 0) { // If Info
        if (playlist_num == playlistcnt) { // If Info for this entry
          inx = metaline.indexOf (",");             // Comma in this line?
          if (inx > 0) {
            // Show artist and title if present in metadata
            showStreamDataOnTFT (metaline.substring (inx + 1).c_str(), true);
          }
        }
      }

      if (metaline.startsWith ("#")) { // // If Commentline
        metalinebfx = 0;                              // ignore
        return;                                       // Ignore commentlines
      }

      // Now we have an URL for a .mp3 file or stream.  Is it the rigth one?
      dbgprint ("Entry %d in playlist found: %s", playlistcnt, metalinebf);
      if (playlist_num == playlistcnt ) {
        inx = metaline.indexOf ("http://");         // Search for "http://"
        if (inx >= 0) { // Does URL contain "http://"?
          host = metaline.substring (inx + 7);      // remove it and set host
        } else {
          host = metaline;                            // set new host
        }
        connectToWebradioServer();                              // Connect to it
      }

      metalinebfx = 0;                                // Prepare for next line
      host = playlist;                                // Back to the .m3u host
      playlistcnt++;                                  // Next entry in playlist
    }
  }
}




/**
 * @desc	Called from the main loop() for the mp3 functions.
 * 			A connection to an MP3 server is active and we are ready to receive data.
 * 			Normally there is about 2 to 4 kB available in the data stream.  This depends on the sender.
 * */
void mp3loop() {
  uint32_t        maxchunk;                             // Max number of bytes to read
  int             res = 0;                              // Result reading from mp3 stream
  uint32_t        av = 0;                               // Available in stream
  String          nodeID;                               // Next nodeID of track on SD
  uint32_t        timing;                               // Startime and duration this function
  uint32_t        qspace;                               // Free space in data queue

  // Try to keep the Queue to playStreamFromQueue filled up by adding as much bytes as possible
  // Check if playing
  if (datamode & (INIT | HEADER | DATA | METADATA | PLAYLISTINIT | PLAYLISTHEADER | PLAYLISTDATA)){
    timing = millis();                                  // Start time this function
    maxchunk = sizeof(tmpbuff);                         // Reduce byte count for this mp3loop()
    qspace = uxQueueSpacesAvailable( dataqueue) *       // Compute free space in data queue
             sizeof(qdata_struct);

    if (localfile) { // If playing file from SD card
      av = mp3filelength;                               // Bytes left in file

      // Reduce byte count for this mp3loop()
      if (av < maxchunk){
        maxchunk = av;
      }

      if (maxchunk > qspace) { // Check if there is enough space in queue
        maxchunk = qspace;                              // No, limit to free queue space
      }

      if (maxchunk) { // Check if there is anything to read
        claimSPI ("sdread");                          // Claim SPI bus
        res = mp3file.read (tmpbuff, maxchunk);       // Read a block of data
        releaseSPI();                                   // Release SPI bus
        mp3filelength -= res;                           // Number of bytes left
      }
    } else { // Play next
      av = mp3client.available();                       // Available from stream

      if (av < maxchunk) { // Limit read size
        maxchunk = av;
      }

      if (maxchunk > qspace) { // Check if there is enough space in queue
        maxchunk = qspace;                              // No, limit to free queue space
      }

      // Check if there is anything to read
      if (maxchunk) {
        res = mp3client.read (tmpbuff, maxchunk);     // Read a number of bytes from the stream
      } else {
        if (datamode == PLAYLISTDATA) { // If End of playlist
          playlist_num = 0;                             // And reset
          dbgprint ("End of playlist seen");
          datamode = STOPPED;
          ini_block.newpreset++;                        // Go to next preset
        }
      }
    }

    for (int i = 0; i < res; i++) {
      handleNextDataByteFromServer (tmpbuff[i]);                     // Handle one byte
    }

    timing = millis() - timing;                         // Duration this function
    if (timing > max_mp3loop_time) { // New maximum found
      max_mp3loop_time = timing;                        // set new maximum
      dbgprint ("Duration mp3loop %d", timing);       // and report it
    }
  }

  if (datamode == STOPREQD) { // If STOP requested
    dbgprint ("STOP requested");
    if (localfile) {
      claimSPI ("close");                             // Claim SPI bus
      mp3file.close();
      releaseSPI();                                     // Release SPI bus
    } else {
      stop_mp3client();                                 // Disconnect if still connected
    }

    chunked = false;                                    // Not longer chunked
    datacount = 0;                                      // Reset datacount
    outqp = outchunk.buf;                               // and pointer
    queueFunctionForPlayTask (QSTOPSONG);                            // Queue a request to stop the song
    metaint = 0;                                        // No metaint known now
    datamode = STOPPED;                                 // state becomes STOPPED
    return;
  }

  if (localfile) { // If playing from SD-Card
    if (datamode & DATA) { // Test if playing
       // End of mp3 data
      if (av == 0) {
        datamode = STOPREQD;                            // End of local mp3-file detected
        nodeID = selectnextSDnode (SD_currentnode,
                                    +1);               // Select the next file on SD
        host = getSDfilename (nodeID);
        hostreq = true;                                 // Request this host
      }
    }
  }

  if (ini_block.newpreset != currentpreset) { // If New station or next from playlist requested
    if (datamode != STOPPED) { // If still busy stop the request
      datamode = STOPREQD;
    } else {
      if (playlist_num) { // If playing from playlist

    	// retrieve URL of playlist
        playlist_num += ini_block.newpreset -
                        currentpreset;                   // Next entry in playlist
        ini_block.newpreset = currentpreset;             // Stay at current preset
      } else {
        host = nvsGetHostFromPref();                       // Lookup preset in preferences
        chomp (host);                                  // Get rid of part after "#"
      }

      dbgprint ("New preset/file requested (%d/%d) from %s",
	  ini_block.newpreset, playlist_num, host.c_str());

      if (host != "") { // if Preset in ini-file
        hostreq = true;                                  // Force this station as new preset
      } else {
        // This preset is not available, return to preset 0, will be handled in next mp3loop()
        dbgprint ("No host for this preset");
        ini_block.newpreset = 0;                         // Wrap to first station
      }
    }
  }

  if (hostreq) { // New preset or station
    hostreq = false;
    currentpreset = ini_block.newpreset;                 // Remember current preset
    // Find out if this URL is on localhost (SD).
    localfile = (host.indexOf ("localhost/") >= 0);
    if (localfile) { // If playing file from localhost
      if (connecttofile()) { // open mp3-file
        datamode = DATA;                                 // Start in DATA mode
      }
    } else {
      if (host.startsWith ("ihr/")) { // If iHeartRadio station requested
        host = host.substring (4);                     // remove "ihr/"
        host = xmlgethost (host);                      // Parse the xml to get the host
      }

      connectToWebradioServer();                                   // Switch to new host
    }
  }

}



/**
 * @desc	Disconnect from the server
 * */
void stop_mp3client (){
  while(mp3client.connected()) {
    dbgprint("Stopping client");               // Stop connection to host
    mp3client.flush();
    mp3client.stop();
    delay(500);
  }
  mp3client.flush();                              // Flush stream client
  mp3client.stop();                               // Stop stream client
}
