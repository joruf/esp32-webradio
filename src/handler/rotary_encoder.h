/**
 * @brief Rotary encoder handling: volume, preset selection, and track selection.
 * 
 * This function is called regularly to process encoder clicks, double-clicks,
 * rotations, and inactivity timeouts. It controls power toggling, preset
 * selection, volume adjustment, and (optionally) track selection from SD card.
 * 
 * @note The original logic is kept exactly as in the working version to ensure
 *       continuous volume control after power toggle.
 */
void checkRotaryEncoder()
{
  static int8_t  enc_preset ;
  static String  enc_nodeID ;
  static String  enc_filename ;
  String         tmp ;
  int16_t        inx ;

  // Inactivity timer: return to VOLUME mode after 4 seconds
  if ( enc_menu_mode != VOLUME )
  {
    if ( enc_inactivity > 40 )
    {
      enc_inactivity = 0 ;
      enc_menu_mode = VOLUME ;
      dbgprint ( "Encoder mode back to VOLUME" ) ;
      showTextOnTFT ( 2, "" ) ; // Clear display line
    }
  }

  // Activate backlight and reset timer on any activity
  if ( singleclick || doubleclick || tripleclick || longclick || rotationcount != 0 ) {
    enableBacklight ( true ) ;
    bltimer = 0 ;
  } else {
    return ;
  }

  // --- Single click handling (power toggle in VOLUME mode) ---
  if (singleclick) {
    // In VOLUME mode: radio on/off (power toggle)
    if (enc_menu_mode == VOLUME) {
      singleclick = false;
      dbgprint("Click in volume mode - Power Toggle");

      if (datamode != STOPPED) { 
        stopStream();
        showTextOnTFT(1, "Radio aus"); 
        showTextOnTFT(3, "Ausschalten..."); 
      } 
      else {
        if (enc_preset < 0 || enc_preset > MAXPRESETS) enc_preset = 0;
        ini_block.newpreset = enc_preset; 
        playlist_num = 0;
        currentpreset = -1; 
        showTextOnTFT(1, "Radio an");
        showTextOnTFT(3, "Verbinde...");
        dbgprint("Starte Preset %d", enc_preset);
      }
    } 
    // In other modes (PRESET/TRACK), the click is processed later as confirmation
  }

  // Double click: activate preset selection mode
  if ( doubleclick )
  {
    dbgprint ( "Double click") ;
    doubleclick = false ;
    enc_menu_mode = PRESET ;
    showTextOnTFT (3, msgTurnToSelectStation);
    enc_preset = ini_block.newpreset;
  }

  // Single click confirmation for PRESET / TRACK mode
  if ( singleclick )
  {
    dbgprint ( "Single click (Mode confirmation)") ;
    singleclick = false ;
    switch ( enc_menu_mode )
    {
      case PRESET :
        currentpreset = -1 ;
        ini_block.newpreset = enc_preset ;
        enc_menu_mode = VOLUME ;
        showTextOnTFT ( 1, "Radio an" ) ;
        showTextOnTFT ( 3, "Sender gewählt" ) ;
        break ;
      case TRACK :
        host = enc_filename ;
        hostreq = true ;
        enc_menu_mode = VOLUME ;
        showTextOnTFT ( 3, "" ) ;
        break ;
      default:
        // VOLUME case is already handled above
        break;
    }
  }

  // Long click: stop or random SD track (currently disabled)
  if ( longclick )
  {
    /*
    longclick = false;
    dbgprint ( "Long click") ;
    if ( datamode != STOPPED) {
      stopStream();
      showTextOnTFT(1, "Radio gestoppt");
    } else {
      if (SD_nodecount) {
        host = getSDfilename ( "0" );
        hostreq = true;
      }
    }
    */
  }

  // Triple click: SD track mode (currently disabled)
  /*
  if ( tripleclick )
  {
    dbgprint ( "Triple click") ;
    tripleclick = false ;
    if ( SD_nodecount )
    {
      enc_menu_mode = TRACK ;
      showTextOnTFT ( 3, "Track wählen..." ) ;
      enc_nodeID = selectnextSDnode ( SD_currentnode, +1 ) ;
      if ( datamode != STOPPED ) datamode = STOPREQD ;
    }
  }
  */

  // Rotation handling (volume, preset, or track selection)
  if ( rotationcount == 0 ) return ;
  
  switch ( enc_menu_mode )
  {
    case VOLUME :
      ini_block.reqvol = constrain(ini_block.reqvol + rotationcount, 0, 100);
      muteflag = false ;
      break ;
      
    case PRESET :
      enc_preset += rotationcount ;
      if ( enc_preset < 0 ) enc_preset = MAXPRESETS;
      tmp = nvsGetHostFromPref ( enc_preset ) ;
      if ( tmp == "" ) {
        enc_preset = 0 ;
        tmp = nvsGetHostFromPref ( enc_preset ) ;
      }
      inx = tmp.indexOf ( "#" ) ;
      if ( inx > 0 ) tmp.remove ( 0, inx + 1 ) ;
      chomp ( tmp ) ;
      showTextOnTFT ( 3, tmp ) ;
      break ;
      
    case TRACK :
      enc_nodeID = selectnextSDnode ( enc_nodeID, rotationcount ) ;
      enc_filename = getSDfilename ( enc_nodeID ) ;
      tmp = enc_filename ;
      while ( ( inx = tmp.indexOf ( "/" ) ) >= 0 ) tmp.remove ( 0, inx + 1 ) ;
      showTextOnTFT ( 3, tmp ) ;
      break ;
  }
  rotationcount = 0 ;
}