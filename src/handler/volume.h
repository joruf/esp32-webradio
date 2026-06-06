
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
 * @desc	Check if stream is running
 * */
bool isStreamPlaying () {
	return datamode != STOPPED;
}


/**
 * @desc	Play or mute the sound
 * */
void playSound (bool _enable) {
	if (_enable == true) {
		muteflag = false; // Mute/unmute  boolean{mute=true, play=false}
		showTextOnTFT(3, icyname); // Set screen segment bottom part
	} else {
		muteflag = true; // Mute/unmute  boolean{mute=true, play=false}
		showTextOnTFT(3, "Mute"); // Set screen segment bottom part
	}
}


/**
 * @brief Controls audio playback (start/stop).
 * 
 * This function starts or stops the audio stream based on the 'play' parameter.
 * It updates the display accordingly and manages the backlight.
 * 
 * @details
 * - If play = true: Starts playback using the specified preset or current preset
 * - If play = false: Stops the current playback
 * - Updates the TFT display to show current status (Stream started/stopped, Radio on/off)
 * - Controls backlight (enabled when playing, disabled when stopped)
 * 
 * @param _runStream If true, start playback; if false, stop playback. Default is true.
 * @param _preset The preset number to play (0 = use current preset, >0 = specific preset).
 *                Only used when play = true.
 * 
 * @note The function does not toggle - it explicitly starts or stops based on the 'play' parameter.
 * 
 * @see datamode (global) - Current playback state (STOPPED = not playing)
 * @see stopStream() - Function to stop the current stream
 * @see playStream() - Function to start a stream with a given preset
 * @see showTextOnTFT() - Display text on the TFT screen
 * @see enableBacklight() - Control display backlight
 */
bool runStream(bool _runStream, int16_t _preset = 0) {
	if (!_runStream) { // datamode != STOPPED
		dbgprint("Stream stopping.");
		
		datamode = STOPREQD;
	    //muteflag = true;   // Sound off, tuner off

		 // Display info
		showTextOnTFT(1, "Stream stopped");
		showTextOnTFT(3, "Radio off");

		enableBacklight(false);
		return true;
	} else {
		dbgprint("Stream starting.");
	
		if (_preset > MAXPRESETS | _preset < 0) {
			return false;
		}

		//hostreq = true;  // Force immediate connection establishment
		muteflag = false;  // sound on
		ini_block.newpreset = _preset;
		playlist_num = 0;
		currentpreset = -1;


		// Display info
	   showTextOnTFT(1, "Stream stared");
	   showTextOnTFT(3, "Radio on");

	   enableBacklight(true);
	
	   return true;
	}
}