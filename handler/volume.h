
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


