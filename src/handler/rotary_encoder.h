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
  static int8_t  enc_preset = 0;
  String         tmp;
  int16_t        inx;

  // Inactivity timer: return to VOLUME mode after 4 seconds
  if ( enc_menu_mode != VOLUME )
  {
    if ( enc_inactivity > 40 )
    {
      enc_inactivity = 0;
      enc_menu_mode = VOLUME;
      dbgprint( "Encoder mode back to VOLUME" );
      showTextOnTFT( 2, "" );
    }
  }

  // Activate backlight on any activity
  if ( singleclick || longclick || doubleclick || rotationcount != 0 )
  {
    enableBacklight( true );
    bltimer = 0;
  }
  else
  {
    return;
  }

  // --- Long click: enter preset selection mode (process FIRST) ---
  if ( longclick )
  {
    dbgprint( "Long click detected - Enter preset selection mode" );
    longclick = false;           // Clear immediately
    enc_menu_mode = PRESET;
    showTextOnTFT( 3, msgTurnToSelectStation );
    enc_preset = ini_block.newpreset;
    return;                      // Exit, don't process single click in same cycle
  }

  // --- Single click: depends on current mode (only if longclick was false) ---
  if ( singleclick )
  {
    singleclick = false;
    if ( enc_menu_mode == VOLUME )
    {
      // Power toggle
      dbgprint( "Single click - Power toggle" );
      if ( datamode != STOPPED )
      {
        runStream(false);
      }
      else
      {
        
        if ( enc_preset < 0 || enc_preset > MAXPRESETS )
        {
          enc_preset = 0;
        }
        ini_block.newpreset = enc_preset;
        runStream(true, currentpreset); 
        playlist_num = 0;
        currentpreset = -1;
      }
    }
    else if ( enc_menu_mode == PRESET )
    {
      // Confirm selected preset and return to volume mode
      dbgprint( "Single click - Confirm preset" );
      currentpreset = -1;
      ini_block.newpreset = enc_preset;
      enc_menu_mode = VOLUME;
      showTextOnTFT( 1, "Radio an" );
      showTextOnTFT( 3, "Sender gewählt" );
    }
    // TRACK mode is not used
  }

  // --- Rotation handling ---
  if ( rotationcount == 0 ) return;

  switch ( enc_menu_mode )
  {
    case VOLUME:
      ini_block.reqvol = constrain( ini_block.reqvol + rotationcount, 0, 100 );
      muteflag = false;
      break;

    case PRESET:
      enc_preset += rotationcount;
      if ( enc_preset < 0 ) enc_preset = MAXPRESETS;
      tmp = nvsGetHostFromPref( enc_preset );
      if ( tmp == "" )
      {
        enc_preset = 0;
        tmp = nvsGetHostFromPref( enc_preset );
      }
      inx = tmp.indexOf( "#" );
      if ( inx > 0 ) tmp.remove( 0, inx + 1 );
      chomp( tmp );
      showTextOnTFT( 3, tmp );
      break;

    default:
      break;
  }
  rotationcount = 0;
}