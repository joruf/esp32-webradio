//**************************************************************************************************
//                                      H A N D L E V O L P U B                                    *
//**************************************************************************************************
// Handle publish of Volume to MQTT.  This will happen max every 10 seconds.                       *
//**************************************************************************************************
void handleVolume() {
  static uint32_t pubtime = 10000 ;                        // Limit save to once per 10 seconds
  static uint8_t  oldvol = -1 ;                            // For comparison

  if ( ( millis() - pubtime ) < 10000 ) { // 10 seconds
    return ;
  }
  pubtime = millis() ;                                     // Set time of last publish
  if ( ini_block.reqvol != oldvol ) { // Volume change?
    mqttpub.trigger ( MQTT_VOLUME ) ;                      // Request publish VOLUME
    oldvol = ini_block.reqvol ;                            // Remember publishe volume
  }
}



/**
 * @desc	Check if sound is playing
 * */
bool isSoundPlaying () {
	if (muteflag == false) {
		return true;
	}

	return false;
}


/**
 * @desc	Run or stop the current defined stream
 * */
void stopStream () {
	datamode = STOPREQD ;                     // Stop player
}


/**
 * @desc	Run the defined stream with the preset number _preset
 * */
bool playStream (int16_t _preset) {
	if (_preset > MAXPRESETS) {
		return false;
	}

	ini_block.newpreset = _preset;               // adjust currentpreset (play this preset number)
    playlist_num = 0 ;                            // Absolute, reset playlist
    currentpreset = -1 ;                          // Make sure current is different

	true;
}


/**
 * @desc	Play or mute the sound
 * */
void playSound (bool _enable) {
	if (_enable == true) {
		muteflag = false;                                // Mute/unmute  boolean{mute=true, play=false}
		showTextOnTFT(3, icyname);                          // Set screen segment bottom part
	} else {
		muteflag = true;                               // Mute/unmute  boolean{mute=true, play=false}
		showTextOnTFT(3, "Mute");                          // Set screen segment bottom part
	}
}


