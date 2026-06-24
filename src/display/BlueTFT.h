// bluetft.h
//
// Separated from the main sketch to allow several display types.
// Includes for various ST7735 displays.  Size is 160 x 128.  Select
// INITR_BLACKTAB for this and set dsp_getwidth() to 160. Works also for the 128
// x 128 version.  Select INITR_144GREENTAB for this and set dsp_getwidth() to
// 128.

#include "St7735Display.h"
#include <Adafruit_ST7735.h>

// Color definitions for the TFT screen (if used)
// TFT has bits 6 bits (0..5) for RED, 6 bits (6..11) for GREEN and 4 bits
// (12..15) for BLUE.
#define BLACK ST7735_BLACK
#define BLUE ST7735_BLUE
#define RED ST7735_RED
#define GREEN ST7735_GREEN
#define CYAN GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW RED | GREEN
#define WHITE RED | BLUE | GREEN

// Data to display.  There are TFTSECS sections
#define TFTSECS 4
scrseg_struct tftdata[TFTSECS] = // Screen divided in 3 segments + 1 overlay
    {
        // One text line is 8 pixels
        {false, WHITE, 0, 8, ""},    // 1 top line
        {false, CYAN, 20, 64, ""},   // 8 lines in the middle
        {false, YELLOW, 90, 32, ""}, // 4 lines at the bottom
        {false, GREEN, 90, 32, ""}   // 4 lines at the bottom for rotary encoder
};

St7735Display st7735Display;
Adafruit_ST7735 *tft = NULL;

// Various macro's to mimic the ST7735 version of display functions
#define dsp_setRotation() tft->setRotation(1)                     // Use landscape format (3 for upside down)
#define dsp_print(a) tft->print(a)                                // Print a string
#define dsp_println(b) tft->println(b)                            // Print a string followed by newline
#define dsp_fillRect(a, b, c, d, e) tft->fillRect(a, b, c, d, e); // Fill a rectange
#define dsp_setTextSize(a) tft->setTextSize(a)                    // Set the text size
#define dsp_setTextColor(a) tft->setTextColor(a)                  // Set the text color
#define dsp_setCursor(a, b) tft->setCursor(a, b)                  // Position the cursor
#define dsp_erase() tft->fillScreen(BLACK);                       // Clear the screen
#define dsp_getwidth() 160                                        // Adjust to your display
#define dsp_getheight() 128                                       // Get height of screen
#define dsp_update()                                              // Updates to the physical screen
#define dsp_isUseSPI() true                                       // Does use SPI

#include "dsp_common.h"

bool dsp_begin() {
  if (!st7735Display.begin()) {
    return false;
  }
  tft = st7735Display.panel();
  if (tft != NULL) {
    displaytype = T_BLUETFT;
  }
  return (tft != NULL);
}

//**************************************************************************************************
//                                      D I S P L A Y B A T T E R Y *
//**************************************************************************************************
// Show the current battery charge level on the screen. * It will overwrite the
// top divider.                                                              *
// No action if bat0/bat100 not defined in the preferences. *
//**************************************************************************************************
void displayBatteryChargeOnTFT() {
  dspCommonBatteryBar(tftdata, dsp_getwidth(), tft != nullptr);
}

//**************************************************************************************************
//                                      D I S P L A Y V O L U M E *
//**************************************************************************************************
// Show the current volume as an indicator on the screen. * The indicator is 2
// pixels heigh. *
//**************************************************************************************************
void displayVolumeOnTFT() {
  if (tft) {
    dspCommonVolumeBar(dsp_getwidth(), dsp_getheight(), getOutputVolume(), true);
  }
}

/**
 * @brief Show stream activity as a horizontal VU bar below the top line.
 *
 * @param level Activity level 0-100
 * @param visible When false, hide the bar
 */
void displayVuMeterOnTFT(uint8_t level, bool visible) {
  if (tft) {
    dspCommonVuMeterBar(dsp_getwidth(), level, true, visible);
  }
}

//**************************************************************************************************
//                                      D I S P L A Y T I M E *
//**************************************************************************************************
// Show the time on the LCD at a fixed position in a specified color * To
// prevent flickering, only the changed part of the timestring is displayed. *
// An empty string will force a refresh on next call. * A character on the
// screen is 8 pixels high and 6 pixels wide. *
//**************************************************************************************************
//**************************************************************************************************
//                                      D I S P L A Y T I M E *
//**************************************************************************************************
// Zeigt Tag, Datum und Uhrzeit auf dem LCD an.
//**************************************************************************************************
//**************************************************************************************************
//                                      D I S P L A Y T I M E *
//**************************************************************************************************
// Zeigt Tag, Datum und Uhrzeit rechtsbündig auf dem LCD an.
//**************************************************************************************************
//**************************************************************************************************
//                                      D I S P L A Y T I M E *
//**************************************************************************************************
// Displays day, date and time right-aligned on the LCD screen.
//**************************************************************************************************
void showTimeOnTFT(const char *str, uint16_t color) {
  static char oldStr[40] = ""; // Buffer for string comparison
  uint8_t i;                   // Index for loops

  // DYNAMIC START POSITION FOR RIGHT-ALIGNMENT:
  // dsp_getwidth() is 160. One character is 6 pixels wide.
  // We subtract the string width and keep a 4-pixel margin from the right edge.
  uint16_t pos = dsp_getwidth() - (strlen(str) * 6) - 4;

  if (str[0] == '\0') // Empty string forces a refresh
  {
    memset(oldStr, '\0', sizeof(oldStr));
    oldStr[sizeof(oldStr) - 1] = '\0';
    return;
  }

  if (tft) // Is TFT active?
  {
    // Safety clearing: clear the space before the new starting point
    // if the previous string was longer than the current one.
    static uint16_t lastPos = 0;
    if (lastPos > 0 && pos > lastPos) {
      dsp_fillRect(lastPos, 0, pos - lastPos, 8, BLACK);
    }
    lastPos = pos;

    dsp_setTextColor(color); // Set requested text color

    // Loop through the new formatted string
    for (i = 0; str[i] != '\0' && i < 39; i++) {
      if (str[i] != oldStr[i]) // Character changed?
      {
        dsp_fillRect(pos, 0, 6, 8, BLACK); // Clear old character
        dsp_setCursor(pos, 0);             // Set cursor position
        dsp_print(str[i]);                 // Print new character
        oldStr[i] = str[i];                // Update history buffer
      }
      pos += 6; // Move 6 pixels right for next char
    }

    // If the new string is shorter than the old one, clear the remaining
    // characters
    while (oldStr[i] != '\0' && i < 39) {
      dsp_fillRect(pos, 0, 6, 8, BLACK);
      oldStr[i] = '\0';
      pos += 6;
      i++;
    }
  }
}
