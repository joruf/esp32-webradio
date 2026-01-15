//**************************************************************************************************
//                                           C H K _ E N C                                         *
//**************************************************************************************************
// See if rotary encoder is activated and perform its functions.                                   *
//**************************************************************************************************

enum SINGLE_CLICK_STATUS {STOPPED_PLAYING, SELECT_PRESET, PLAYING_PRESET};
SINGLE_CLICK_STATUS singleClickStatus = PLAYING_PRESET;


void checkRotaryEncoder()
{
  static int8_t  enc_preset ;                                 // Selected preset
  static String  enc_nodeID ;                                 // Node of selected track
  static String  enc_filename ;                               // Filename of selected track
  String         tmp ;                                        // Temporary string
  int16_t        inx ;                                        // Position in string

  if ( enc_menu_mode != VOLUME )                              // In default mode?
  {
    if ( enc_inactivity > 40 )                                // No, more than 4 seconds inactive
    {
      enc_inactivity = 0 ;
      enc_menu_mode = VOLUME ;                                // Return to VOLUME mode
      dbgprint ( "Encoder mode back to VOLUME" ) ;
      showTextOnTFT ( 2, (char*)NULL ) ;                             // Restore original text at bottom
    }
  }

  if ( singleclick || doubleclick || tripleclick || longclick || rotationcount != 0 ) { // Any activity?
    enableBacklight ( true ) ;                                          // activate display if needed
  } else {
    return ;                                                  // No, nothing to do
  }


  // The singelClickStatus changes in a loop from: PLAYING_PRESET -> STOPPED_PLAYING -> SELECT_PRESET -> PLAYING_PRESET -> ...
  if (singleclick) {
	  singleclick = false; // We have to say, that we now not in the single click mode!
	    dbgprint("Rotary Encoder Single Click") ;

	  switch (singleClickStatus) {
		  case PLAYING_PRESET:
			  dbgprint("PLAYING_PRESET");
	    	  playStream(enc_preset); 								//Start the channel again

			  enc_menu_mode = VOLUME ;                              // Back to the default rotary encoder mode
	    	  singleClickStatus = STOPPED_PLAYING;
			  break;
		  case STOPPED_PLAYING:
			  dbgprint("STOPPED_PLAYING");
			  //playSound(false);
			  stopStream();
			  //currentpreset = -1;
		   	  showTextOnTFT(1, "");
			  showTextOnTFT(3, msgPlayerStopped);
			  // @todo There is any bug. on stopping mp3.h line 422 is executing, nothing works since yet

			  enc_menu_mode = VOLUME ;                              // Back to the default rotary encoder mode
			  singleClickStatus = SELECT_PRESET;
			  break;
		  case SELECT_PRESET:
			  dbgprint("SELECT_PRESET");
			  dbgprint ( "Encoder mode set to PRESET" ) ;
			  showTextOnTFT (3, msgTurnToSelectStation);

			  enc_preset = ini_block.newpreset;                    // Start with current preset

			  enc_menu_mode = PRESET;                                  // Swich to PRESET mode
			  singleClickStatus = PLAYING_PRESET;
			  break;
	  }

  }



  /*
  if ( tripleclick )                                          // First handle triple click
  {
    dbgprint ( "Triple click") ;
    tripleclick = false ;
    if ( SD_nodecount )                                       // Tracks on SD?
    {
      enc_menu_mode = TRACK ;                                 // Swich to TRACK mode
      dbgprint ( "Encoder mode set to TRACK" ) ;
      showTextOnTFT ( 3, "Turn to select track\n"                    // Show current option
               "Press to confirm" ) ;
      enc_nodeID = selectnextSDnode ( SD_currentnode, +1 ) ;  // Start with next file on SD
      if ( enc_nodeID == "" )                                 // Current track available?
      {
        inx = SD_nodelist.indexOf ( "\n" ) ;                  // No, find first
        enc_nodeID = SD_nodelist.substring ( 0, inx ) ;
      }
      // Stop playing as reading filenames saturates SD I/O.
      if ( datamode != STOPPED )
      {
        datamode = STOPREQD ;                                 // Request STOP
      }
    }
  }

  if ( doubleclick )                                          // Handle the doubleclick
  {
    dbgprint ( "Double click") ;
    doubleclick = false ;
    enc_menu_mode = PRESET ;                                  // Swich to PRESET mode
    dbgprint ( "Encoder mode set to PRESET" ) ;
    showTextOnTFT (3, msgTurnToSelectStation);
    enc_preset = ini_block.newpreset;                    // Start with current preset
  }

  if ( singleclick )
  {
    dbgprint ( "Single click") ;
    singleclick = false ;
    switch ( enc_menu_mode )                                  // Which mode (VOLUME, PRESET, TRACK)?
    {
      case VOLUME :
        if (isSoundPlaying() == false)
        {
          playSound(true);
          //showTextOnTFT ( 3, "" ) ;                                  // Clear text
          //showTextOnTFT ( 2, icyname ) ;                      // Set screen segment bottom part
        } else {
          playSound(false);
          //showTextOnTFT ( 3, "Mute" ) ;
        }
        //muteflag = !muteflag ;                                // Mute/unmute
        break ;
      case PRESET :
        currentpreset = -1 ;                                  // Make sure current is different
        ini_block.newpreset = enc_preset ;                    // Make a definite choice
        enc_menu_mode = VOLUME ;                              // Back to default mode
        showTextOnTFT ( 3, "" ) ;                                    // Clear text
        break ;
      case TRACK :
        host = enc_filename ;                                 // Selected track as new host
        hostreq = true ;                                      // Request this host
        enc_menu_mode = VOLUME ;                              // Back to default mode
        showTextOnTFT ( 3, "" ) ;                                    // Clear text
        break ;
    }
  }

  if ( longclick )                                            // Check for long click
  {
    dbgprint ( "Long click") ;
    if ( datamode != STOPPED) {
      datamode = STOPREQD ;                                   // Request STOP, do not touch longclick flag
   	  showTextOnTFT(1, "");
	  showTextOnTFT(3, msgPlayerStopped);
    } else {
      longclick = false ;                                     // Reset condition
      dbgprint ( "Long click detected" ) ;
      if (SD_nodecount) {									// Tracks on SD?
        host = getSDfilename ( "0" ) ;                        // Get random track
        hostreq = true ;                                      // Request this host
      }
      muteflag = false ;                                      // Be sure muteing is off
    }
  }
  */

  if ( rotationcount == 0 )                                   // Any rotation?
  {
    return ;                                                  // No, return
  }
  dbgprint ( "Rotation count %d", rotationcount ) ;
  switch ( enc_menu_mode )                                    // Which mode (VOLUME, PRESET, TRACK)?
  {
    case VOLUME :
      if ( ( ini_block.reqvol + rotationcount ) < 0 )         // Limit volume
      {
        ini_block.reqvol = 0 ;                                // Limit to normal values
      }
      else if ( ( ini_block.reqvol + rotationcount ) > 100 )
      {
        ini_block.reqvol = 100 ;                              // Limit to normal values
      }
      else
      {
        ini_block.reqvol += rotationcount ;
      }
      muteflag = false ;                                      // Mute off
      break ;
    case PRESET :
      if ( ( enc_preset + rotationcount ) < 0 )               // Negative not allowed
      {
        enc_preset = 0 ;                                      // Stay at 0
      } else {
        enc_preset += rotationcount ;                         // Next preset
      }
      tmp = nvsGetHostFromPref ( enc_preset ) ;                 // Get host spec and possible comment
      if ( tmp == "" )                                        // End of presets?
      {
        enc_preset = 0 ;                                      // wrap
        tmp = nvsGetHostFromPref ( enc_preset ) ;               // Get host spec and possible comment
      }
      dbgprint ( "Preset is %d", enc_preset ) ;
      // Show just comment if available.  Otherwise the preset itself.
      inx = tmp.indexOf ( "#" ) ;                             // Get position of "#"
      if ( inx > 0 )                                          // Hash sign present?
      {
        tmp.remove ( 0, inx + 1 ) ;                           // remove non-comment part
      }
      chomp ( tmp ) ;                                         // Remove garbage from description
      showTextOnTFT ( 3, tmp ) ;                                     // Set screen segment bottom part
      break ;
    case TRACK :
      enc_nodeID = selectnextSDnode ( enc_nodeID,
                                      rotationcount ) ;       // Select the next file on SD
      enc_filename = getSDfilename ( enc_nodeID ) ;           // Set new filename
      tmp = enc_filename ;                                    // Copy for display
      dbgprint ( "Select %s", tmp.c_str() ) ;
      while ( ( inx = tmp.indexOf ( "/" ) ) >= 0 )            // Search for last slash
      {
        tmp.remove ( 0, inx + 1 ) ;                           // Remove before the slash
      }
      dbgprint ( "Simplified %s", tmp.c_str() ) ;
      showTextOnTFT ( 3, tmp ) ;
    // Set screen segment bottom part
    default :
      break ;
  }
  rotationcount = 0 ;                                         // Reset
}
