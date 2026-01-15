




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

  if (ini_block.alrm_start.c_str() != "none") {
    int8_t alrm_tm_start_hour = getValue(ini_block.alrm_start.c_str(), ':', 0).toInt();
    int8_t alrm_tm_start_min = getValue(ini_block.alrm_start.c_str(), ':', 1).toInt();
    if (/*radioAlarmOn == false &&*/ timeinfo.tm_hour == alrm_tm_start_hour && timeinfo.tm_min == alrm_tm_start_min) {   // Start radio alarm
      playSound(true);
      playStream(currentpreset);
      enableBacklight(true);
      showTextOnTFT(1, msgRadioAlarmOn);                    // Set screen segment bottom part
      radioAlarmStatusChanged = ALRM_START;
    }
  }

  if (ini_block.alrm_stop.c_str() != "none") {
    //showTextOnTFT(3, ini_block.alrm_stop.c_str());
    int8_t alrm_tm_stop_hour = getValue(ini_block.alrm_stop.c_str(), ':', 0).toInt();
    int8_t alrm_tm_stop_min = getValue(ini_block.alrm_stop.c_str(), ':', 1).toInt();
    if (/*radioAlarmOn == true &&*/ timeinfo.tm_hour == alrm_tm_stop_hour && timeinfo.tm_min == alrm_tm_stop_min) {   // Stop radio alarm
      stopStream();
      //playSound(false);
      enableBacklight(true);
      showTextOnTFT(1, msgRadioAlarmOff);                   // Set screen segment bottom part
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
