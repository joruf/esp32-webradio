
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

void stopStream() {
    datamode = STOPREQD;
    //muteflag = true;   // Ton aus, Verstärker deaktivieren
}

bool playStream(int16_t _preset) {
    if (_preset > MAXPRESETS) return false;
    ini_block.newpreset = _preset;
    playlist_num = 0;
    currentpreset = -1;
    //muteflag = false;  // Ton wieder an
    //hostreq = true;    // Sofortigen Verbindungsaufbau erzwingen
    return true;
}



