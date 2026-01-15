



//**************************************************************************************************
//                                      T F T S E T                                                *
//**************************************************************************************************
// Request to display a segment on TFT.  Version for char* and String parameter.                   *
//**************************************************************************************************
void showTextOnTFT(uint16_t inx, const char *str )
{
  if(inx < TFTSECS)                                 // Segment available on display
  {
    if(str)                                         // String specified?
    {
      tftdata[inx].str = String(str);               // set string
    }
    tftdata[inx].update_req = true ;                    // and request flag
  }
}

void showTextOnTFT(uint16_t inx, String& str )
{
  if(inx < TFTSECS)                                 // Segment available on display
  {
    tftdata[inx].str = str ;                            // Set string
    tftdata[inx].update_req = true ;                    // and request flag
  }
}






//**************************************************************************************************
//                                           BACKLIGHT  S E T                                      *
//**************************************************************************************************
// Enable or disable the TFT backlight if configured.                                              *
// May be called from interrupt level.                                                             *
//**************************************************************************************************
void IRAM_ATTR enableBacklight(bool enable)
{
  if(ini_block.tft_bl_pin >= 0) {                     // Backlight for TFT control?
    digitalWrite(ini_block.tft_bl_pin, enable);      // Enable/disable backlight
  }

  if(ini_block.tft_blx_pin >= 0) {                    // Backlight for TFT (inversed logic) control?
    digitalWrite(ini_block.tft_blx_pin, !enable);    // Enable/disable backlight
  }

  if(enable) {
    bltimer = 0;                                        // Reset counter backlight time-out
  }
}





//**************************************************************************************************
//                                           T F T L O G                                           *
//**************************************************************************************************
// Log to TFT if enabled.                                                                          *
//**************************************************************************************************
void tftlog(const char *str)
{
  if(tft)                                           // TFT configured?
  {
    dsp_println(str);                              // show error on TFT
    dsp_update();                                     // To physical screen
  }
}


//**************************************************************************************************
//                                      D I S P L A Y I N F O                                      *
//**************************************************************************************************
// Show a string on the LCD at a specified y-position (0..2) in a specified color.                 *
// The parameter is the index in tftdata[].                                                        *
//**************************************************************************************************
void displayinfo(uint16_t inx)
{
  uint16_t       width = dsp_getwidth();                  // Normal number of colums
  scrseg_struct* p = &tftdata[inx];
  uint16_t len;                                           // Length of string, later buffer length

  if(inx == 0)                                          // Topline is shorter
  {
    width += TIMEPOS;                                     // Leave space for time
  }
  if(tft)                                               // TFT active?
  {
    dsp_fillRect(0, p->y, width, p->height, BLACK);    // Clear the space for new info
    if(( dsp_getheight() > 64) &&(p->y > 1))        // Need and space for divider?
    {
      dsp_fillRect(0, p->y - 4, width, 1, GREEN);      // show divider above text
    }
    len = p->str.length();                                // Required length of buffer
    if(len++)                                           // Check string length, set buffer length
    {
      char buf [ len ];                                   // Need some buffer space
      p->str.toCharArray(buf, len);                    // Make a local copy of the string
      utf8ascii(buf);                                  // Convert possible UTF8
      dsp_setTextColor(p->color);                      // Set the requested color
      dsp_setCursor(0, p->y);                          // Prepare to show the info
      dsp_println(buf);                                // Show the string
    }
  }
}






//**************************************************************************************************
//                                H A N D L E _ T F T _ T X T                                      *
//**************************************************************************************************
// Check if tft refresh is requested.                                                              *
//**************************************************************************************************
bool handle_tft_txt()
{
  for(uint16_t i = 0; i < TFTSECS; i++)              // Handle all sections
  {
    if(tftdata[i].update_req)                          // Refresh requested?
    {
      displayinfo(i);                                 // do the refresh
      dsp_update();                                      // Updates to the screen
      tftdata[i].update_req = false;                     // Reset request
      return true;                                       // Just handle 1 request
    }
  }
  return false;                                          // Not a single request
}
