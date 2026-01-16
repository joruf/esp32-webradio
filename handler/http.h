//**************************************************************************************************
//                                      F I L L K E Y L I S T                                      *
//**************************************************************************************************
// Fill the list of all relevant keys in NVS.                                                      *
// The keys will be sorted.                                                                        *
//**************************************************************************************************
void fillkeylist()
{
  esp_err_t    result = ESP_OK ;                                // Result of reading partition
  uint32_t     offset = 0 ;                                     // Offset in nvs partition
  uint16_t     i ;                                              // Index in Entry 0..125.
  uint8_t      bm ;                                             // Bitmap for an entry
  uint16_t     nvsinx = 0 ;                                     // Index in nvskey table

  keynames.clear() ;                                            // Clear the list
  while ( offset < nvs->size )
  {
    result = esp_partition_read ( nvs, offset, &nvsbuf, sizeof(nvsbuf) ) ; // Read 1 page in nvs partition
    if ( result != ESP_OK )
    {
      dbgprint ( "Error reading NVS!" ) ;
      break ;
    }
    i = 0 ;
    while ( i < 126 )
    {
      bm = ( nvsbuf.Bitmap[i / 4] >> ( ( i % 4 ) * 2 ) ) ;      // Get bitmap for this entry,
      bm &= 0x03 ;                                              // 2 bits for one entry
      if ( bm == 2 )                                            // Entry is active?
      {
        if ( nvsbuf.Entry[i].Ns == namespace_ID )               // Namespace right?
        {
          strcpy ( nvskeys[nvsinx], nvsbuf.Entry[i].Key ) ;     // save in table
          if ( ++nvsinx == MAXKEYS )
          {
            nvsinx-- ;                                          // Prevent excessive index
          }
        }
        i += nvsbuf.Entry[i].Span ;                             // Next entry
      }
      else
      {
        i++ ;
      }
    }
    offset += sizeof(nvs_page) ;                                // Prepare to read next page in nvs
  }
  nvskeys[nvsinx][0] = '\0' ;                                   // Empty key at the end
  dbgprint ( "Read %d keys from NVS", nvsinx ) ;
  bubbleSortKeys ( nvsinx ) ;                                   // Sort the keys
}




//**************************************************************************************************
//                                           M K _ L S A N                                         *
//**************************************************************************************************
// Make al list of acceptable networks in preferences.                                             *
// Will be called only once by setup().                                                            *
// The result will be stored in wifilist.                                                          *
// Not that the last found SSID and password are kept in common data.  If only one SSID is         *
// defined, the connect is made without using wifiMulti.  In this case a connection will           *
// be made even if de SSID is hidden.                                                              *
//**************************************************************************************************
void  mk_lsan()
{
  uint8_t     i ;                                        // Loop control
  char        key[10] ;                                  // For example: "wifi_03"
  String      buf ;                                      // "SSID/password"
  String      lssid, lpw ;                               // Last read SSID and password from nvs
  int         inx ;                                      // Place of "/"
  WifiInfo_t  winfo ;                                    // Element to store in list

  dbgprint ( "Create list with acceptable WiFi networks" ) ;
  for ( i = 0 ; i < 100 ; i++ )                          // Examine wifi_00 .. wifi_99
  {
    sprintf ( key, "wifi_%02d", i ) ;                    // Form key in preferences
    if ( nvsKeyExists ( key  ) )                            // Does it exists?
    {
      buf = nvsGetValue ( key ) ;                          // Get the contents
      inx = buf.indexOf ( "/" ) ;                        // Find separator between ssid and password
      if ( inx > 0 )                                     // Separator found?
      {
        lpw = buf.substring ( inx + 1 ) ;                // Isolate password
        lssid = buf.substring ( 0, inx ) ;               // Holds SSID now
        dbgprint ( "Added %s to list of networks",
                   lssid.c_str() ) ;
        winfo.inx = i ;                                  // Create new element for wifilist ;
        winfo.ssid = strdup ( lssid.c_str() ) ;          // Set ssid of element
        winfo.passphrase = strdup ( lpw.c_str() ) ;
        wifilist.push_back ( winfo ) ;                   // Add to list
        wifiMulti.addAP ( winfo.ssid,                    // Add to wifi acceptable network list
                          winfo.passphrase ) ;
      }
    }
  }
  dbgprint ( "End adding networks" ) ;
}



//**************************************************************************************************
//                                     G E T R A D I O S T A T U S                                 *
//**************************************************************************************************
// Return preset-, tone- and volume status.                                                        *
// Included are the presets, the current station, the volume and the tone settings.                *
//**************************************************************************************************
String getradiostatus()
{
  char                pnr[3] ;                           // Preset as 2 character, i.e. "03"

  sprintf ( pnr, "%02d", ini_block.newpreset ) ;         // Current preset
  return String ( "preset=" ) +                          // Add preset setting
         String ( pnr ) +
         String ( "\nvolume=" ) +                        // Add volume setting
         String ( String ( ini_block.reqvol ) ) +
         String ( "\ntoneha=" ) +                        // Add tone setting HA
         String ( ini_block.rtone[0] ) +
         String ( "\ntonehf=" ) +                        // Add tone setting HF
         String ( ini_block.rtone[1] ) +
         String ( "\ntonela=" ) +                        // Add tone setting LA
         String ( ini_block.rtone[2] ) +
         String ( "\ntonelf=" ) +                        // Add tone setting LF
         String ( ini_block.rtone[3] ) ;
}



//**************************************************************************************************
//                                        R I N B Y T                                              *
//**************************************************************************************************
// Read next byte from http inputbuffer.  Buffered for speed reasons.                              *
//**************************************************************************************************
uint8_t rinbyt ( bool forcestart )
{
  static uint8_t  buf[1024] ;                           // Inputbuffer
  static uint16_t i ;                                   // Pointer in inputbuffer
  static uint16_t len ;                                 // Number of bytes in buf
  uint16_t        tlen ;                                // Number of available bytes
  uint16_t        trycount = 0 ;                        // Limit max. time to read

  if ( forcestart || ( i == len ) )                     // Time to read new buffer
  {
    while ( cmdclient.connected() )                     // Loop while the client's connected
    {
      tlen = cmdclient.available() ;                    // Number of bytes to read from the client
      len = tlen ;                                      // Try to read whole input
      if ( len == 0 )                                   // Any input available?
      {
        if ( ++trycount > 3 )                           // Not for a long time?
        {
          dbgprint ( "HTTP input shorter than expected" ) ;
          return '\n' ;                                 // Error! No input
        }
        delay ( 10 ) ;                                  // Give communication some time
        continue ;                                      // Next loop of no input yet
      }
      if ( len > sizeof(buf) )                          // Limit number of bytes
      {
        len = sizeof(buf) ;
      }
      len = cmdclient.read ( buf, len ) ;               // Read a number of bytes from the stream
      i = 0 ;                                           // Pointer to begin of buffer
      break ;
    }
  }
  return buf[i++] ;
}


//**************************************************************************************************
//                                        W R I T E P R E F S                                      *
//**************************************************************************************************
// Update the preferences.  Called from the web interface.                                         *
// Reads the values from the interface															   *
//**************************************************************************************************
void writeprefs()
{
  int        inx ;                                            // Position in inputstr
  uint8_t    winx ;                                           // Index in wifilist
  char       c ;                                              // Input character
  String     inputstr = "" ;                                  // Input regel
  String     key, contents ;                                  // Pair for Preferences entry
  String     dstr ;                                           // Contents for debug

  /*
  timerAlarmDisable ( timer ) ;                               // Disable the timer
  */
  timerStop ( timer ) ;                                       // Disable the timer

  nvsClear() ;                                                // Remove all preferences
  while ( true )
  {
    c = rinbyt ( false ) ;                                    // Get next inputcharacter
    if ( c == '\n' )                                          // Newline?
    {
      if ( inputstr.length() == 0 )



      
      {
        dbgprint ( "End of writing preferences" ) ;
        break ;                                               // End of contents
      }
      if ( !inputstr.startsWith ( "#" ) )                     // Skip pure comment lines
      {
        inx = inputstr.indexOf ( "=" ) ;
        if ( inx >= 0 )                                       // Line with "="?
        {
          key = inputstr.substring ( 0, inx ) ;               // isolate the key
          key.trim() ;
          contents = inputstr.substring ( inx + 1 ) ;         // and contents
          contents.trim() ;
          dstr = contents ;                                   // Copy for debug
          if ( ( key.indexOf ( "wifi_" ) == 0 ) )             // Sensitive info?
          {
            winx = key.substring(5).toInt() ;                 // Get index in wifilist
            if ( ( winx < wifilist.size() ) &&                // Existing wifi spec in wifilist?
                 ( contents.indexOf ( wifilist[winx].ssid ) == 0 ) &&
                 ( contents.indexOf ( "/****" ) > 0 ) )       // Hidden password?
            {
              contents = String ( wifilist[winx].ssid ) +     // Retrieve ssid and password
                         String ( "/" ) +
                         String ( wifilist[winx].passphrase ) ;
              dstr = String ( wifilist[winx].ssid ) +
                     String ( "/*******" ) ;                  // Hide in debug line
            }
          }
          if ( ( key.indexOf ( "mqttpasswd" ) == 0 ) )        // Sensitive info?
          {
            if ( contents.indexOf ( "****" ) == 0 )           // Hidden password?
            {
              contents = ini_block.mqttpasswd ;               // Retrieve mqtt password
            }
            dstr = String ( "*******" ) ;                     // Hide in debug line
          }
          dbgprint ( "writeprefs setstr %s = %s",
                     key.c_str(), dstr.c_str() ) ;
          nvsAddValue ( key.c_str(), contents ) ;               // Save new pair (needs a restart to use it in the program)


          // Save this values directly in the ini_block to make it available without restart!
          if (key.indexOf("alrm_start") >= 0) {
              ini_block.alrm_start = dstr.c_str();
              dbgprint ("Start Alarm at %s", dstr.c_str());
          } else if (key.indexOf("alrm_stop") >= 0) {
              ini_block.alrm_stop = dstr.c_str();
              dbgprint ("Stop Alarm %s", dstr.c_str());
          }

        }
      }
      inputstr = "" ;
    }
    else
    {
      if ( c != '\r' )                                        // Not newline.  Is is a CR?
      {
        inputstr += String ( c ) ;                            // No, normal char, add to string
      }
    }
  }

  /*
  timerAlarmEnable ( timer ) ;                                // Enable the timer
  */
  timerStart ( timer ) ;                                // Enable the timer
  
  fillkeylist() ;                                             // Update list with keys
}


//**************************************************************************************************
//                                     G E T S E T T I N G S                                       *
//**************************************************************************************************
// Send some settings to the webserver.                                                            *
// Included are the presets, the current station, the volume and the tone settings.                *
//**************************************************************************************************
void getsettings()
{
  String              val ;                              // Result to send
  String              statstr ;                          // Station string
  int                 inx ;                              // Position of search char in line
  int16_t             i ;                                // Loop control, preset number

  for ( i = 0 ; i < MAXPRESETS ; i++ )                   // Max number of presets
  {
    statstr = nvsGetHostFromPref ( i ) ;   				         // Get the preset from NVS
    if ( statstr != "" )								                 // Preset available?
    {
      // Show just comment if available.  Otherwise the preset itself.
      inx = statstr.indexOf ( "#" ) ;                    // Get position of "#"
      if ( inx > 0 )                                     // Hash sign present?
      {
        statstr.remove ( 0, inx + 1 ) ;                  // remove non-comment part
      }
      chomp ( statstr ) ;                                // Remove garbage from description
      dbgprint ( "statstr is %s", statstr.c_str() ) ;
      val += String ( "preset_" ) +
             String ( i ) +
             String ( "=" ) +
             statstr +
             String ( "\n" ) ;                           // Add delimeter
      if ( val.length() > 1000 )                         // Time to flush?
      {
        cmdclient.print ( val ) ;                        // send
        val = "" ;                                       // Start new string
      }
    }
  }
  val += getradiostatus() +                              // Add radio setting
         String ( "\n\n" ) ;                             // End of reply
  cmdclient.print ( val ) ;                              // And send
}


//**************************************************************************************************
//                                        H A N D L E H T T P R E P L Y                            *
//**************************************************************************************************
// Handle the output after an http request.                                                        *
//**************************************************************************************************
void handlehttpreply()
{
  const char*   p ;                                         // Pointer to reply if command
  String        sndstr = "" ;                               // String to send
  int           n ;                                         // Number of files on SD card

  if ( http_response_flag )
  {
    http_response_flag = false ;
    if ( cmdclient.connected() )
    {
      if ( http_rqfile.length() == 0 &&                     // An empty "GET"?
           http_getcmd.length() == 0 )
      {
        if ( NetworkFound )                                 // check network
        {
          handleWebpage ( String( "index.html") ) ;             // Okay, send the startpage
        }
        else
        {
          handleWebpage ( String( "config.html") ) ;            // Or the configuration page if in AP mode
        }
      }
      else
      {
        if ( http_getcmd.length() )                         // Command to analyze?
        {
          dbgprint ( "Send reply for %s", http_getcmd.c_str() ) ;
          sndstr = httpheader ( String ( "text/html" ) ) ;  // Set header

          if ( http_getcmd.startsWith ( "getprefs" ) )      // Is it a "Get preferences"?
          {
            if ( datamode != STOPPED )                      // Still playing?
            {
//              datamode = STOPREQD ;                         // Stop playing
            }
            sndstr += readprefs ( true ) ;                  // Read and send
          }
          else if ( http_getcmd.startsWith ( "getdefs" ) )  // Is it a "Get default preferences"?
          {
            sndstr += String ( defprefs_txt + 1 ) ;         // read initial values
          }
          else if ( http_getcmd.startsWith ("saveprefs") )  // Is is a "Save preferences"
          {
            writeprefs() ;                                  // handle it
            sndstr += String ( "Config saved" ) ;           // Give reply
          }
          else if ( http_getcmd.startsWith ( "mp3list" ) )  // Is is a "Get SD MP3 tracklist"?
          {
            if ( datamode != STOPPED )                      // Still playing?
            {
//              datamode = STOPREQD ;                         // Stop playing
            }
            cmdclient.print ( sndstr ) ;                    // send header
            n = listsdtracks ( "/" ) ;                      // Handle it
            dbgprint ( "%d tracks found on SD card", n ) ;
            return ;                                        // Do not send empty line
          }
          else if ( http_getcmd.startsWith ( "settings" ) ) // Is is a "Get settings" (like presets and tone)?
          {
            cmdclient.print ( sndstr ) ;                    // send header
            getsettings() ;                                 // Handle settings request
            return ;                                        // Do not send empty line
          }
          else
          {
            p = analyzeCmd ( http_getcmd.c_str() ) ;        // do so
            sndstr += String ( p ) ;                        // Content of HTTP response follows the header
          }
          sndstr += String ( "\n" ) ;                       // The HTTP response ends with a blank line
          cmdclient.print ( sndstr ) ;
        }
        else if ( http_rqfile.length() )                    // File requested?
        {
          dbgprint ( "Start file reply for %s",
                     http_rqfile.c_str() ) ;
          handleWebpage ( http_rqfile ) ;                       // send it
        }
        else
        {
          httpheader ( String ( "text/html" ) ) ;           // Send header
          // the content of the HTTP response follows the header:
          cmdclient.println ( "Dummy response\n" ) ;        // Text ending with double newline
          dbgprint ( "Dummy response sent" ) ;
        }
      }
    }
  }
}


//**************************************************************************************************
//                                        H A N D L E H T T P                                      *
//**************************************************************************************************
// Handle the input of an http request.                                                            *
//**************************************************************************************************
void handlehttp()
{
  bool        first = true ;                                 // First call to rinbyt()
  char        c ;                                            // Next character from http input
  int         inx0, inx ;                                    // Pos. of search string in currenLine
  String      currentLine = "" ;                             // Build up to complete line
  bool        reqseen = false ;                              // No GET seen yet

  if ( !cmdclient.connected() )                              // Action if client is connected
  {
    return ;                                                 // No client active
  }
  dbgprint ( "handlehttp started" ) ;
  while ( true )                                             // Loop till command/file seen
  {
    c = rinbyt ( first ) ;                                   // Get a byte
    first = false ;                                          // No more first call
    if ( c == '\n' )
    {
      // If the current line is blank, you got two newline characters in a row.
      // that's the end of the client HTTP request, so send a response:
      if ( currentLine.length() == 0 )
      {
        http_response_flag = reqseen ;                       // Response required or not
        break ;
      }
      else
      {
        // Newline seen, remember if it is like "GET /xxx?y=2&b=9 HTTP/1.1"
        if ( currentLine.startsWith ( "GET /" ) )            // GET request?
        {
          inx0 = 5 ;                                         // Start search at pos 5
        }
        else if ( currentLine.startsWith ( "POST /" ) )      // POST request?
        {
          inx0 = 6 ;
        }
        else
        {
          inx0 = 0 ;                                         // Not GET nor POST
        }
        if ( inx0 )                                          // GET or POST request?
        {
          reqseen = true ;                                   // Request seen
          inx = currentLine.indexOf ( "&" ) ;                // Search for 2nd parameter
          if ( inx < 0 )
          {
            inx = currentLine.indexOf ( " HTTP" ) ;          // Search for end of GET command
          }
          // Isolate the command
          http_getcmd = currentLine.substring ( inx0, inx ) ;
          inx = http_getcmd.indexOf ( "?" ) ;                // Search for command
          if ( inx == 0 )                                    // Arguments only?
          {
            http_getcmd = http_getcmd.substring ( 1 ) ;      // get rid of question mark
            http_rqfile = "" ;                               // No file
          }
          else if ( inx > 0 )                                // Filename present?
          {
            http_rqfile = http_getcmd.substring ( 0, inx ) ; // Remember filename
            http_getcmd = http_getcmd.substring ( inx + 1 ) ; // Remove filename from GET command
          }
          else
          {
            http_rqfile = http_getcmd ;                      // No parameters, set filename
            http_getcmd = "" ;
          }
          if ( http_getcmd.length() )
          {
            dbgprint ( "Get command is: %s",                 // Show result
                       http_getcmd.c_str() ) ;
          }
          if ( http_rqfile.length() )
          {
            dbgprint ( "Filename is: %s",                    // Show requested file
                       http_rqfile.c_str() ) ;
          }
        }
        currentLine = "" ;
      }
    }
    else if ( c != '\r' )                                    // No LINFEED.  Is it a CR?
    {
      currentLine += c ;                                     // No, add normal char to currentLine
    }
  }

  // Sendet Sendername, Songtitel, Uhrzeit und Datum als JSON
  if (http_getcmd.startsWith("getJSON")) {
    char time_str[10];  // Platz für "HH:MM:SS\0"
    char date_str[12];  // Platz für "TT.MM.JJJJ\0"

    // Falls noch keine Zeit vom NTP-Server geholt wurde (tm_year ist dann 0)
    if (timeinfo.tm_year > 0) {
      sprintf(time_str, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
      sprintf(date_str, "%02d.%02d.%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
    } else {
      strcpy(time_str, "00:00:00");
      strcpy(date_str, "01.01.2026"); // Oder Platzhalter
    }

    // JSON zusammenbauen
    String jsonResponse = "{";
    jsonResponse += "\"station\":\"" + icyname + "\",";
    jsonResponse += "\"title\":\"" + icystreamtitle + "\",";
    jsonResponse += "\"firmware\":\"" + String(FIRMWARE) + "\",";
    jsonResponse += "\"time\":\"" + String(time_str) + "\",";
    jsonResponse += "\"date\":\"" + String(date_str) + "\"";
    jsonResponse += "}";

    // Header für JSON senden
    cmdclient.print("HTTP/1.1 200 OK\r\n");
    cmdclient.print("Content-Type: application/json; charset=utf-8\r\n");
    cmdclient.print("Content-Length: "); cmdclient.print(jsonResponse.length()); cmdclient.print("\r\n");
    cmdclient.print("Access-Control-Allow-Origin: *\r\n");
    cmdclient.print("Connection: close\r\n");
    cmdclient.print("\r\n"); 
    
    cmdclient.print(jsonResponse); 
    
    cmdclient.flush();
    cmdclient.stop(); 
    
    http_getcmd = ""; 
    http_rqfile = "";
    http_response_flag = false; 
    return; 
  }


  //cmdclient.stop() ;
}


//**************************************************************************************************
//                                      H A N D L E I P P U B                                      *
//**************************************************************************************************
// Handle publish op IP to MQTT.  This will happen every 10 minutes.                               *
//**************************************************************************************************
void handleIpPub()
{
  static uint32_t pubtime = 300000 ;                       // Limit save to once per 10 minutes

  if ( ( millis() - pubtime ) < 600000 )                   // 600 sec is 10 minutes
  {
    return ;
  }
  pubtime = millis() ;                                     // Set time of last publish
  mqttpub.trigger ( MQTT_IP ) ;                            // Request re-publish IP
}



//**************************************************************************************************
//                                     H T T P H E A D E R                                         *
//**************************************************************************************************
// Set http headers to a string.                                                                   *
//**************************************************************************************************
String httpheader ( String contentstype )
{
  return String ( "HTTP/1.1 200 OK\nContent-type:" ) +
         contentstype +
         String ( "\n"
                  "Server: " NAME "\n"
                  "Cache-Control: " "max-age=3600\n"
                  "Last-Modified: " FIRMWARE "\n\n" ) ;
}
