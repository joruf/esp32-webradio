//**************************************************************************************************
//                                         G E T T I M E                                           *
//**************************************************************************************************
// Retrieve the local time from NTP server and convert to string.                                  *
// Will be called every second.                                                                    *
//**************************************************************************************************
//**************************************************************************************************
//                                         G E T T I M E                                           *
//**************************************************************************************************
// Retrieve the local time from NTP server and convert to string.                                  *
// Will be called every second.                                                                    *
//**************************************************************************************************
void getCurrentTime()
{
  static int16_t delaycount = 0 ;                           // To reduce number of NTP requests
  static int16_t retrycount = 100 ;
  
  static const char* days[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

  if ( tft )                                                // TFT used?
  {
    if ( timeinfo.tm_year )                                 // Legal time found?
    {
      // Format: "Sa 06.06.26 13:34" (Exactly 17 characters)
      sprintf(timetxt, "%s %02d.%02d.%02d %02d:%02d", 
              days[timeinfo.tm_wday], 
              timeinfo.tm_mday, 
              timeinfo.tm_mon + 1,        // tm_mon runs from 0 to 11
              timeinfo.tm_year % 100,     // Two-digit year
              timeinfo.tm_hour, 
              timeinfo.tm_min);
    }
    
    if ( --delaycount <= 0 )                                // Sync every few hours
    {
      delaycount = 7200 ;                                   // Reset counter
      if ( timeinfo.tm_year )                               // Legal time found?
      {
        dbgprint ( "Sync TOD, old value is %s", timetxt ) ;
      }
      dbgprint ( "Sync TOD" ) ;
      if ( !getLocalTime ( &timeinfo ) )                    // Read from NTP server
      {
        dbgprint ( "Failed to obtain time!" ) ;             // Error
        timeinfo.tm_year = 0 ;                              // Set current time to illegal
        if ( retrycount )                                   // Give up syncing?
        {
          retrycount-- ;                                    // No, try again
          delaycount = 5 ;                                  // Retry after 5 seconds
        }
      }
      else
      {
        // Apply the same short format after successful NTP sync
        sprintf(timetxt, "%s %02d.%02d.%02d %02d:%02d", 
                days[timeinfo.tm_wday], 
                timeinfo.tm_mday, 
                timeinfo.tm_mon + 1, 
                timeinfo.tm_year % 100, 
                timeinfo.tm_hour, 
                timeinfo.tm_min);
      }
    }
  }
}