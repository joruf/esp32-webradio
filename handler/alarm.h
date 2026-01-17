int32_t sleeptimer_ms = -1; // -1 bedeutet Timer ist aus


void handleSleepTimer() {
  static uint32_t last_millis = 0;
  
  if (sleeptimer_ms > 0) {
    uint32_t current_millis = millis();
    uint32_t diff = current_millis - last_millis;
    last_millis = current_millis;

    sleeptimer_ms -= diff;

    if (sleeptimer_ms <= 0) {
      sleeptimer_ms = -1; // Timer zurücksetzen
      if (datamode != STOPPED) {
        dbgprint("Sleep Timer abgelaufen. Stoppe Radio.");
        stopStream(); // Funktion aus volume.h
        showTextOnTFT(1, "Sleep Timer"); // Info auf Display
        showTextOnTFT(3, "Radio aus");
      }
    }
  } else {
    last_millis = millis(); // Timer steht, Zeitbasis aktuell halten
  }
}



// Enable/Disable radio alarm at specified time
// timeinfo.tm_hour|tm_min|tm_sec|tm_year
void handleRadioAlarm () {
  if (radioAlarmStatusChanged == ALRM_START || radioAlarmStatusChanged == ALRM_STOP) { // Update the icyname and radio alarm info one minute after alarm
	  radioAlarmStatusChanged = ALRM_NONE;
	  //showTextOnTFT(1, "");

	  if (radioAlarmStatusChanged == ALRM_START) {
		  showTextOnTFT(3, icyname);                          // Set screen segment bottom part
	  } else {
		  //showTextOnTFT(3, "");
	  }
  }

  // --- RADIO ALARM START ---
  if (ini_block.alrm_start != "none" && ini_block.alrm_start.length() >= 4) {
    int8_t alrm_tm_start_hour = getValue(ini_block.alrm_start.c_str(), ':', 0).toInt();
    int8_t alrm_tm_start_min = getValue(ini_block.alrm_start.c_str(), ':', 1).toInt();

    if (timeinfo.tm_hour == alrm_tm_start_hour && timeinfo.tm_min == alrm_tm_start_min && timeinfo.tm_sec == 0) {
      // 1. Wir nehmen den eingestellten Alarm-Sender (ini_block.newpreset)
      playStream(ini_block.newpreset); 

      // 2. Das Signal geben, dass der MP3-Loop sofort loslegen soll
      // (Das passiert im Hintergrund auch beim Web-Klick)
      hostreq = true; 

      // 3. Ton einschalten
      muteflag = false; 

      // 4. Optik
      enableBacklight(true);
      showTextOnTFT(1, msgRadioAlarmOn);
      radioAlarmStatusChanged = ALRM_START;
    }
  }

  // --- RADIO ALARM STOP ---
  if (ini_block.alrm_stop.c_str() != "none" && ini_block.alrm_stop.length() >= 4) {
    int8_t alrm_tm_stop_hour = getValue(ini_block.alrm_stop.c_str(), ':', 0).toInt();
    int8_t alrm_tm_stop_min = getValue(ini_block.alrm_stop.c_str(), ':', 1).toInt();
    
    // NEU: Zusätzliche Prüfung auf Sekunden (0 bis 2)
    if (timeinfo.tm_hour == alrm_tm_stop_hour && 
        timeinfo.tm_min == alrm_tm_stop_min && 
        timeinfo.tm_sec <= 2) {   // Nur in den ersten 2 Sekunden der Minute ausführen
        
      stopStream();
      enableBacklight(true);
      showTextOnTFT(1, msgRadioAlarmOff);
      radioAlarmStatusChanged = ALRM_STOP;
      showTextOnTFT(3, msgPlayerStopped);
    }
  }

  //    if (ini_block.alrm_start) {
        //String val = nvsGetValue("alarmclocktime");              // Get the clock time from memory
  //    }


//  dbgprint ("Alarm clock time start hour:%d min:%d", alrm_tm_start_hour, alrm_tm_start_min);
//  dbgprint ("Alarm clock time stop hour:%d min:%d", alrm_tm_stop_hour, alrm_tm_stop_min);
//dbgprint("Clock time, %d:%d", timeinfo.tm_hour, timeinfo.tm_min);
    //showTextOnTFT (3, "Alarm on");    // Print a message on TFT {0=top, 1=middle, 2-3=bottom}
    //muteflag = !muteflag;                                // Mute/unmute  boolean{mute=false, play=true}
//    showTextOnTFT(2, icyname );                      // Set screen segment bottom part

}
