//**************************************************************************************************
//                                           C H K _ E N C                                         *
//**************************************************************************************************
// See if rotary encoder is activated and perform its functions.                                   *
//**************************************************************************************************

enum SINGLE_CLICK_STATUS {STOPPED_PLAYING, SELECT_PRESET, PLAYING_PRESET};
SINGLE_CLICK_STATUS singleClickStatus = PLAYING_PRESET;


void checkRotaryEncoder()
{
  static int8_t  enc_preset ;
  static String  enc_nodeID ;
  static String  enc_filename ;
  String         tmp ;
  int16_t        inx ;

  // Inaktivitäts-Timer: Zurück zum Volume-Modus nach 4 Sekunden
  if ( enc_menu_mode != VOLUME )
  {
    if ( enc_inactivity > 40 )
    {
      enc_inactivity = 0 ;
      enc_menu_mode = VOLUME ;
      dbgprint ( "Encoder mode back to VOLUME" ) ;
      showTextOnTFT ( 2, "" ) ; // Anzeige bereinigen
    }
  }

  // Backlight aktivieren bei Aktivität
  if ( singleclick || doubleclick || tripleclick || longclick || rotationcount != 0 ) {
    enableBacklight ( true ) ;
  } else {
    return ;
  }

  // --- DER ENTSCHEIDENDE TEIL: SINGLE CLICK LOGIK ---
  if (singleclick) {
    // Wenn wir im VOLUME Modus sind -> RADIO AN/AUS (Power Toggle)
    if (enc_menu_mode == VOLUME) {
      singleclick = false; // Klick hier verarbeiten
      dbgprint("Klick im Volume-Modus - Power Toggle");

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
    // Wenn wir in einem anderen Modus sind (PRESET oder TRACK)
    // lassen wir singleclick = true stehen, damit es weiter unten 
    // im originalen Switch-Block verarbeitet wird!
  }

  // Double Click: Senderwahl-Modus aktivieren
  if ( doubleclick )
  {
    dbgprint ( "Double click") ;
    doubleclick = false ;
    enc_menu_mode = PRESET ;
    showTextOnTFT (3, msgTurnToSelectStation);
    enc_preset = ini_block.newpreset;
  }

  // Single Click Verarbeitung für PRESET / TRACK Bestätigung
  if ( singleclick )
  {
    dbgprint ( "Single click (Modus-Bestätigung)") ;
    singleclick = false ;
    switch ( enc_menu_mode )
    {
      case PRESET :
        currentpreset = -1 ;
        ini_block.newpreset = enc_preset ;
        enc_menu_mode = VOLUME ; // Zurück zum Volume nach Auswahl
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
        // VOLUME Fall wird hier ignoriert, da oben bereits abgehandelt
        break;
    }
  }

  // Long Click: Stopp oder SD-Zufall
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

  // Triple Click: SD-Track Modus
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


  // Dreh-Logik (unverändert)
  if ( rotationcount == 0 ) return ;
  
  switch ( enc_menu_mode )
  {
    case VOLUME :
      ini_block.reqvol = constrain(ini_block.reqvol + rotationcount, 0, 100);
      muteflag = false ;
      break ;
      
    case PRESET :
      enc_preset += rotationcount ;
      if ( enc_preset < 0 ) enc_preset = MAXPRESETS; // Wrap around
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
