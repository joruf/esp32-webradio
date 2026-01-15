// OTA-Update: https://github.com/esp8266/Arduino/issues/4400


// If the line contains content-length information: set length (content length counter).
// @TODO muss noch in eine andere Datei veschoben werden
void scan_content_length ( const char* metalinebf )
{
  if ( strstr ( metalinebf, "Content-Length" ) )        // Line contains content length
  {
    char* p;
    clength = atoi ( metalinebf + 15 ) ;                // Yes, set clength
    p = dbgprint ( "Content-Length is %d", clength ) ;      // Show for debugging purposes
    dsp_println(p);                                   // Set screen segment bottom part
    delay(2000);
  }
}


//**************************************************************************************************
//                                           O T A S T A R T                                       *
//**************************************************************************************************
// Update via WiFi has been started by Arduino IDE or update request.                              *
//**************************************************************************************************
void otastart()
{
  char* p;
  p = dbgprint ( "OTA update Started" ) ;
  dsp_println(p);                                   // Set screen segment bottom part
  delay(2000);
}


//**************************************************************************************************
// Update NEXTION image from OTA stream.                                                           *
//**************************************************************************************************
bool do_nextion_update ( uint32_t clength )
{
  bool     res = false ;                                       // Update result
  uint32_t k ;
  int      c ;                                                 // Reply from NEXTION

  if ( nxtserial )                                             // NEXTION active?
  {
    vTaskDelete ( xspftask ) ;                                 // Prevent output to NEXTION
    delay ( 1000 ) ;
    nxtserial->printf ( "\xFF\xFF\xFF" ) ;                     // Empty command
    for ( int i = 0 ; i < 100 ; i++ )                          // Any input seen?
    {
      if ( nxtserial->available() )
      {
        c =  nxtserial->read() ;                               // Read garbage
      }
      delay ( 20 ) ;
    }
    nxtserial->printf ( "whmi-wri %d,9600,0\xFF\xFF\xFF",      // Start upload
                        clength ) ;
    while ( !nxtserial->available() )                          // Any input seen?
    {
      delay ( 20 ) ;
    }
    c =  nxtserial->read() ;                                   // Yes, read the 0x05 ACK
    while ( clength )                                          // Loop for the transfer
    {
      k = clength ;
      if ( k > 4096 )
      {
        k = 4096 ;
      }
      k = otaclient.read ( tmpbuff, k ) ;                      // Read a number of bytes from the stream
      dbgprint ( "TFT file, read %d bytes", k ) ;
      nxtserial->write ( tmpbuff, k ) ;     
      while ( !nxtserial->available() )                        // Any input seen?
      {
        delay ( 20 ) ;
      }
      c =  (char)nxtserial->read() ;                           // Yes, read the 0x05 ACK
      if ( c != 0x05 )
      {
        break ;
      }
      clength -= k ;
    }
    otaclient.flush() ;
    if ( clength == 0 )
    {
      dbgprint ( "Update successfully completed" ) ;
      res = true ;
    }
  }
  return res ;
}


//**************************************************************************************************
// Update software from OTA stream.                                                                *
//**************************************************************************************************
bool do_software_update ( uint32_t clength )
{
  bool res = false ;                                          // Update result  
  if ( Update.begin ( clength ) )                             // Update possible?
  {
    dbgprint ( "Begin OTA update, length is %d", clength ) ;
    if ( Update.writeStream ( otaclient ) == clength )        // writeStream is the real download
    {
      dbgprint ( "Written %d bytes successfully", clength ) ;
    }
    else
    {
      dbgprint ( "Write failed!" ) ;
    }
    if ( Update.end() )                                       // Check for successful flash
    {
      dbgprint( "OTA done" ) ;
      if ( Update.isFinished() )
      {
        dbgprint ( "Update successfully completed" ) ;
        res = true ;                                          // Positive result
      }
      else
      {
        dbgprint ( "Update not finished!" ) ;
      }
    }
    else
    {
      dbgprint ( "Error Occurred. Error %s", Update.getError() ) ;
    }
  }
  else
  {
    // Not enough space to begin OTA
    dbgprint ( "Not enough space to begin OTA" ) ;
    otaclient.flush() ;
  }
  return res ;
}


//**************************************************************************************************
// Update software by download from remote host.                                                   *
//**************************************************************************************************
void update_software ( const char* lstmodkey, const char* updatehost, const char* binfile )
{
  dsp_erase() ;                                      // Clear screen
  dsp_println("OTA Update");
  delay(2000);

  uint32_t    timeout = millis() ;                              // To detect time-out
  String      line ;                                            // Input header line
  String      lstmod = "" ;                                     // Last modified timestamp in NVS
  String      newlstmod ;                                       // Last modified from host

  
  updatereq = false ;                                           // Clear update flag
  otastart() ;                                                  // Show something on screen
  stop_mp3client () ;                                           // Stop input stream
  lstmod = nvsGetValue ( lstmodkey ) ;                            // Get current last modified timestamp

  dsp_erase() ; // Clear screen
  dsp_println("Connecting to host");
  dsp_println(updatehost);
  delay(2000);

  dbgprint ( "Connecting to %s for %s",
              updatehost, binfile ) ;
  if ( !otaclient.connect ( updatehost, 80 ) )                  // Connect to host
  {
    dsp_erase() ; // Clear screen
    dsp_println("Connecting failed");
    dbgprint ( "Connect to updatehost failed!" ) ;
    delay(2000);
    return ;
  }
  
  dsp_erase() ; // Clear screen
  dsp_println("Loading bin file");
  delay(2000);
  otaclient.printf ( "GET %s HTTP/1.1\r\n"
                     "Host: %s\r\n"
                     "Cache-Control: no-cache\r\n"
                     "Connection: close\r\n\r\n",
                     binfile,
                     updatehost ) ;
  while ( otaclient.available() == 0 )                          // Wait until response appears
  {
    if ( millis() - timeout > 20000 ) // 20sec timeout
    {
      dsp_erase() ; // Clear screen
      dsp_println("Connect Timeout");
      dbgprint ( "Connect to Update host Timeout!" ) ;
      delay(2000);
      otaclient.stop() ;
      return ;
    }
  }
  
  // Connected, handle response
  dsp_erase() ; // Clear screen
  while ( otaclient.available() )
  {
    line = otaclient.readStringUntil ( '\n' ) ;                 // Read a line from response    
    line.trim() ;                                               // Remove garbage
    
    dsp_println(line.c_str());
    dbgprint ( line.c_str() ) ;                                 // Debug info
    if ( !line.length() )                                       // End of headers?
    {
      break ;                                                   // Yes, get the OTA started
    }
    
    
    // Check if the HTTP Response is 200.  Any other response is an error.
    if ( line.startsWith ( "HTTP/1.1" ) )                       // 
    {
      if ( line.indexOf ( " 200 " ) < 0 )
      {
        dsp_erase() ; // Clear screen
        dsp_println("Invalid HTTP Status");
        dsp_println("Exit OTA Update");
        dbgprint ( "Got a non 200 status code from server!" ) ;
        delay(2000);
        return ;
      }
    }
    scan_content_length ( line.c_str() ) ;                      // Scan for content_length
    if ( line.startsWith ( "Last-Modified: " ) )                // Timestamp of binary file
    {
      newlstmod = line.substring ( 15 ) ;                       // Isolate timestamp
    }
  }
  // End of headers reached
  if ( newlstmod == lstmod )                                    // Need for update?
  {
    dsp_erase() ; // Clear screen
    dsp_println("No update required");
    dsp_println("Exit OTA Update");
    dbgprint ( "No new version available" ) ;                   // No, show reason
    delay(2000);
    otaclient.flush() ;
    return ;    
  }
  if ( clength > 0 )
  {
    dsp_erase() ; // Clear screen
    dsp_println("Updating sketch");

    if ( strstr ( binfile, ".bin" ) )                           // Update of the sketch?
    {
      if ( do_software_update ( clength ) )                     // Flash updated sketch
      {
        nvsAddValue ( lstmodkey, newlstmod ) ;                    // Update Last Modified in NVS
      }
    }
    if ( strstr ( binfile, ".tft" ) )                           // Update of the NEXTION image?
    {
      if ( do_nextion_update ( clength ) )                      // Flash updated NEXTION
      {
        nvsAddValue ( lstmodkey, newlstmod ) ;                    // Update Last Modified in NVS
      }
    }
  }
  else
  {
    dsp_erase() ; // Clear screen
    dsp_println("Missing content in response");
    dbgprint ( "There was no content in the response" ) ;
    delay(2000);
    otaclient.flush() ;
  }
}

