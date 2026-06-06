/**
 * @brief 60-second timer interrupt (extra watchdog).
 * 
 * This function is called every 60 seconds. It checks the radio alarm
 * and enables/disables it when needed.
 * 
 * @note Runs in interrupt context (IRAM_ATTR)
 * @note Calling time-consuming functions may cause crashes!
 * @see handleRadioAlarm()
 */
void IRAM_ATTR timer60sec()
{
  //dbgprint ("60 seconds over, last minute %d ", timeinfo.tm_min);
  // Enable/Disable radio alarm at specified time
  handleRadioAlarm();
}

/**
 * @brief 10-second timer interrupt (extra watchdog).
 * 
 * This function is called every 10 seconds. It checks whether data is still
 * being sent to the MP3 decoder. If inactivity persists, playback is either
 * restarted or the ESP32 is reset.
 * 
 * @details
 * - Counts 6 calls (60 seconds) then calls timer60sec()
 * - Checks the number of bytes sent in the last 10 seconds
 * - If 0 bytes and more than 10 consecutive failures → ESP.restart()
 * - In playlist mode, the playlist is reset
 * - On repeated failure, the current station is restarted
 * - On successful data transfer, the bitrate is calculated
 * 
 * @note Runs in interrupt context (IRAM_ATTR)
 * @note Calling time-consuming functions may cause crashes!
 * @see timer60sec()
 * @see playStream()
 * @see totalcount (global) - Counter of sent bytes
 */
void IRAM_ATTR timer10sec()
{
  sv int16_t   count60sec = 0 ;                   // Counter for activatie 60 seconds process
  if(++count60sec == 6 )                      // 10 seconds *6times = 60 seconds passed
  {
    timer60sec();                                // do 10 second procedure
    count60sec = 0 ;                              // Reset count
  }

  static uint32_t oldtotalcount = 7321 ;          // Needed for change detection
  static uint8_t  morethanonce = 0 ;              // Counter for succesive fails
  uint32_t        bytesplayed ;                   // Bytes send to MP3 converter


  if(datamode &(INIT | HEADER | DATA | METADATA | PLAYLISTINIT | PLAYLISTHEADER | PLAYLISTDATA)) {         // Test op playing
    bytesplayed = totalcount - oldtotalcount ;    // Nunber of bytes played in the 10 seconds
    oldtotalcount = totalcount ;                  // Save for comparison in next cycle
    if(bytesplayed == 0)                      // Still playing?
    {

      if(morethanonce > 10) {                    // No! Happened too many times?
        ESP.restart();                           // Reset the CPU, probably no return
      }

      if(datamode &(PLAYLISTDATA | PLAYLISTINIT | PLAYLISTHEADER)) { // In playlist mode?
        playlist_num = 0 ;                        // end of playlist
      }

      // if the connection is confused on playing, start the same channel again
      if(( morethanonce > 0)|| (playlist_num > 0)) {                // If happened more than once or playlist is active
  		  //datamode = STOPREQD ;                     // Stop player
    	  runStream(true, currentpreset); 				//Start the channel again
      }
      morethanonce++ ;                            // Count the fails

    }
    else
    {
      //                                          // Data has been send to MP3 decoder
      // Bitrate in kbits/s is bytesplayed / 10 / 1000 * 8
      mbitrate =(bytesplayed + 625)/ 1250 ;   // Measured bitrate
      morethanonce = 0 ;                          // Data seen, reset failcounter
    }
  }
}

/**
 * @brief 100-millisecond timer interrupt.
 * 
 * This function is called every 100 ms in interrupt context. It manages
 * time updates, backlight timeout, encoder inactivity, and detection of
 * single/double/triple clicks of the encoder button.
 * 
 * @details
 * - Counts up to 10 seconds and calls timer10sec()
 * - Every second: Updates timeinfo (sec, min, hour) and sets time_req
 * - Backlight: Turns off display after BACKLIGHT_TIME seconds
 * - Encoder inactivity counter increments (reset by interrupts)
 * - Click detection: After 4 stable cycles, sets singleclick,
 *   doubleclick or tripleclick
 * 
 * @note Runs in interrupt context (IRAM_ATTR)
 * @note No long or blocking operations allowed!
 * @see timer10sec()
 * @see enableBacklight()
 * @see singleclick, doubleclick, tripleclick (global)
 */
void IRAM_ATTR timer100()
{
  sv int16_t   count10sec = 0 ;                   // Counter for activatie 10 seconds process
  sv int16_t   eqcount = 0 ;                      // Counter for equal number of clicks
  sv int16_t   oldclickcount = 0 ;                // To detect difference

  if(++count10sec == 100 )                    // 10 seconds passed?
  {
    timer10sec();                                // do 10 second procedure
    count10sec = 0 ;                              // Reset count
  }
  if(( count10sec % 10)== 0)                // One second over?
  {
    scaniocount = scanios ;                       // TEST*TEST*TEST
    scanios = 0 ;
    if(++timeinfo.tm_sec >= 60)               // update number of seconds
    {
      timeinfo.tm_sec = 0 ;                       // Wrap after 60 seconds
      if(++timeinfo.tm_min >= 60 )
      {
        timeinfo.tm_min = 0 ;                     // Wrap after 60 minutes
        if(++timeinfo.tm_hour >= 24 )
        {
          timeinfo.tm_hour = 0 ;                  // Wrap after 24 hours
        }
      }
    }
    time_req = true ;                             // show current time request

    // In loop.h innerhalb von timer100 -> if(( count10sec % 10)== 0)
    if (bltimer < BACKLIGHT_TIME) {           // Nur hochzählen, wenn Limit noch nicht erreicht
        if (++bltimer == BACKLIGHT_TIME) {    // Wenn Limit genau jetzt erreicht wurde
            enableBacklight(false);           // Display aus
            dbgprint("TFT blanking activated");
        }
    }
  }

  // Handle rotary encoder. Inactivity counter will be reset by encoder interrupt
  if(++enc_inactivity == 36000)               // Count inactivity time
  {
    enc_inactivity = 1000 ;                       // Prevent wrap
  }
  // Now detection of single/double click of rotary encoder switch
  if(clickcount)                              // Any click?
  {
    if(oldclickcount == clickcount)           // stable situation?
    {
      if(++eqcount == 4)                      // Long time stable?
      {
        eqcount = 0 ;
        if(clickcount > 2)                    // Triple click?
        {
          tripleclick = true ;                    // set result
        }
        else if(clickcount == 2)              // Double click?
        {
          doubleclick = true ;                    // set result
        }
        else
        {
          singleclick = true ;                    // Just one click seen
        }
        clickcount = 0 ;                          // Reset number of clicks
      }
    }
    else
    {
      oldclickcount = clickcount ;                // To detect change
      eqcount = 0 ;                               // Not stable, reset count
    }
  }
}

/**
 * @brief Reads and processes commands from the Serial interface.
 * 
 * This function is called regularly in the main loop. It reads characters
 * from the serial input, assembles them into commands (ending with \n or \r),
 * and passes them to analyzeCmd() for execution.
 * 
 * @details
 * - Command echo is not sent (commented out)
 * - When NEXTION is active, commands with prefix "N:" are forwarded there
 * - Commands longer than sizeof(cmd)-2 are discarded
 * - The result from analyzeCmd() is printed via dbgprint()
 * 
 * @note Can be called in main loop context (not in interrupt)
 * @see analyzeCmd()
 * @see nxtserial (global) - For NEXTION override
 */
void scanserial()
{
  static String serialcmd ;                      // Command from Serial input
  char          c ;                              // Input character
  const char*   reply = "" ;                     // Reply string from analyzeCmd
  uint16_t      len ;                            // Length of input string

  while ( Serial.available() )                   // Any input seen?
  {
    c =  (char)Serial.read() ;                   // read the next input character
    //Serial.write ( c ) ;                       // Echo
    len = serialcmd.length() ;                   // Get the length of the current string
    if ( ( c == '\n' ) || ( c == '\r' ) )
    {
      if ( len )
      {
        strncpy ( cmd, serialcmd.c_str(), sizeof(cmd) ) ;
        if ( nxtserial )                         // NEXTION test possible?
        {
          if ( serialcmd.startsWith ( "N:" ) )   // Command meant to test Nextion display?
          {
            nxtserial->printf ( "%s\xFF\xFF\xFF", cmd + 2 ) ;
          }
        }
        reply = analyzeCmd ( cmd ) ;             // Analyze command and handle it
        dbgprint ( reply ) ;                     // Result for debugging
        serialcmd = "" ;                         // Prepare for new command
      }
    }
    if ( c >= ' ' )                              // Only accept useful characters
    {
      serialcmd += c ;                           // Add to the command
    }
    if ( len >= ( sizeof(cmd) - 2 )  )           // Check for excessive length
    {
      serialcmd = "" ;                           // Too long, reset
    }
  }
}

/**
 * @brief Reads and processes commands from the second serial interface (NEXTION).
 * 
 * This function is called regularly in the main loop. It reads commands
 * from the NEXTION display (terminated by 0xFF 0xFF 0xFF) and forwards them
 * to analyzeCmd() for execution.
 * 
 * @details
 * - Commands end with three consecutive 0xFF bytes
 * - Only commands with prefix 0x70 (button pressed) are processed
 * - The prefix is removed before passing the command to analyzeCmd()
 * - Commands that are too long are discarded
 * 
 * @note Can be called in main loop context (not in interrupt)
 * @note Requires active nxtserial object
 * @see analyzeCmd()
 * @see nxtserial (global) - NEXTION serial object
 */
void scanserial2()
{
  static String  serialcmd ;                       // Command from Serial input
  char           c ;                               // Input character
  const char*    reply = "" ;                      // Reply string from analyzeCmd
  uint16_t       len ;                             // Length of input string
  static uint8_t ffcount = 0 ;                     // Counter for 3 tmes "0xFF"

  if ( nxtserial )                                 // NEXTION active?
  {
    while ( nxtserial->available() )               // any input seen?
    {
      c =  (char)nxtserial->read() ;               // read the next input character
      len = serialcmd.length() ;                   // Get the length of the current string
      if ( c == 0xFF )                             // End of command?
      {
        if ( ++ffcount < 3 )                       // 3 times FF?
        {
          continue ;                               // No, continue to read
        }
        ffcount = 0 ;                              // For next command
        if ( len )
        {
          strncpy ( cmd, serialcmd.c_str(), sizeof(cmd) ) ;
          dbgprint ( "NEXTION command seen %02X %s",
                     cmd[0], cmd + 1 ) ;
          if ( cmd[0] == 0x70 )                    // Button pressed?
          { 
            reply = analyzeCmd ( cmd + 1 ) ;       // Analyze command and handle it
            dbgprint ( reply ) ;                   // Result for debugging
          }
          serialcmd = "" ;                         // Prepare for new command
        }
      }
      else if ( c >= ' ' )                         // Only accept useful characters
      {
        serialcmd += c ;                           // Add to the command
      }
      if ( len >= ( sizeof(cmd) - 2 )  )           // Check for excessive length
      {
        serialcmd = "" ;                           // Too long, reset
      }
    }
  }
}

/**
 * @brief Scans digital inputs (GPIOs and touch pins) for edge detection.
 * 
 * This function is called regularly in the main loop (max every 100 ms).
 * It detects HIGH→LOW changes on configured digital inputs and touch pins,
 * then executes the associated command.
 * 
 * @details
 * - Debouncing: At least 100 ms between scans
 * - For digital pins: Only HIGH→LOW transitions trigger commands
 * - For touch pins: THRESHOLD (30) defines touch detection level
 * - Touch pins must be LOW for 3 cycles before being considered "touched"
 * - When detected, analyzeCmd() is called with the associated command
 * 
 * @note Can be called in main loop context (not in interrupt)
 * @note progpin[] and touchpin[] must be initialized
 * @see analyzeCmd()
 * @see progpin (global) - Array with digital pin configurations
 * @see touchpin (global) - Array with touch pin configurations
 */
void scandigital()
{
  static uint32_t oldmillis = 5000 ;                        // To compare with current time
  int             i ;                                       // Loop control
  int8_t          pinnr ;                                   // Pin number to check
  bool            level ;                                   // Input level
  const char*     reply ;                                   // Result of analyzeCmd
  int16_t         tlevel ;                                  // Level found by touch pin
  const int16_t   THRESHOLD = 30 ;                          // Threshold or touch pins

  if ( ( millis() - oldmillis ) < 100 )                     // Debounce
  {
    return ;
  }
  scanios++ ;                                               // TEST*TEST*TEST
  oldmillis = millis() ;                                    // 100 msec over
  for ( i = 0 ; ( pinnr = progpin[i].gpio ) >= 0 ; i++ )    // Scan all inputs
  {
    if ( !progpin[i].avail || progpin[i].reserved )         // Skip unused and reserved pins
    {
      continue ;
    }
    level = ( digitalRead ( pinnr ) == HIGH ) ;             // Sample the pin
    if ( level != progpin[i].cur )                          // Change seen?
    {
      progpin[i].cur = level ;                              // And the new level
      if ( !level )                                         // HIGH to LOW change?
      {
        dbgprint ( "GPIO_%02d is now LOW, execute %s",
                   pinnr, progpin[i].command.c_str() ) ;
        reply = analyzeCmd ( progpin[i].command.c_str() ) ; // Analyze command and handle it
        dbgprint ( reply ) ;                                // Result for debugging
      }
    }
  }
  // Now for the touch pins
  for ( i = 0 ; ( pinnr = touchpin[i].gpio ) >= 0 ; i++ )   // Scan all inputs
  {
    if ( !touchpin[i].avail || touchpin[i].reserved )       // Skip unused and reserved pins
    {
      continue ;
    }
    tlevel = ( touchRead ( pinnr ) ) ;                      // Sample the pin
    level = ( tlevel >= THRESHOLD ) ;                       // True if below threshold
    if ( level )                                            // Level HIGH?
    {
      touchpin[i].count = 0 ;                               // Reset count number of times
    }
    else
    {
      if ( ++touchpin[i].count < 3 )                        // Count number of times LOW
      {
        level = true ;                                      // Not long enough: handle as HIGH
      }
    }
    if ( level != touchpin[i].cur )                         // Change seen?
    {
      touchpin[i].cur = level ;                             // And the new level
      if ( !level )                                         // HIGH to LOW change?
      {
        dbgprint ( "TOUCH_%02d is now %d ( < %d ), execute %s",
                   pinnr, tlevel, THRESHOLD,
                   touchpin[i].command.c_str() ) ;
        reply = analyzeCmd ( touchpin[i].command.c_str() ); // Analyze command and handle it
        dbgprint ( reply ) ;                                // Result for debugging
      }
    }
  }
}