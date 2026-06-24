// ILI9341.h
//
// Contributed by Uksa007@gmail.com
// Separated from the main sketch to allow several display types.
// Includes for various ILI9341 displays.  Tested on 320 x 240.
// Requires Adafruit ILI9341 library, available from library manager.
// Below set your dsp_getwidth() and dsp_getwidth() to suite your display.

#include "ILI9341Display.h"
#include <Adafruit_ILI9341.h>

// Color definitions for the TFT screen (if used)
// TFT has bits 6 bits (0..5) for RED, 6 bits (6..11) for GREEN and 4 bits
// (12..15) for BLUE.
#define BLACK ILI9341_BLACK
#define BLUE ILI9341_BLUE
#define RED ILI9341_RED
#define GREEN ILI9341_GREEN
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

ILI9341Display ili9341Display;
Adafruit_ILI9341 *tft = NULL;

// Various macro's to mimic the ILI9341 version of display functions
#define dsp_setRotation() tft->setRotation(3)                     // Use landscape format (3 for upside down)
#define dsp_print(a) tft->print(a)                                // Print a string
#define dsp_println(b) tft->println(b)                            // Print a string followed by newline
#define dsp_fillRect(a, b, c, d, e) tft->fillRect(a, b, c, d, e); // Fill a rectange
#define dsp_setTextSize(a) tft->setTextSize(a)                    // Set the text size
#define dsp_setTextColor(a) tft->setTextColor(a)                  // Set the text color
#define dsp_setCursor(a, b) tft->setCursor(a, b)                  // Position the cursor
#define dsp_erase() tft->fillScreen(BLACK);                       // Clear the screen
#define dsp_getwidth() 320                                        // Adjust to your display
#define dsp_getheight() 240                                       // Get height of screen
#define dsp_update()                                              // Updates to the physical screen
#define dsp_isUseSPI() true                                       // Does use SPI

#include "dsp_common.h"

bool dsp_begin() {
  if (!ili9341Display.begin()) {
    return false;
  }
  tft = ili9341Display.panel();
  if (tft != NULL) {
    displaytype = T_ILI9341;
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
void showTimeOnTFT(const char *str, uint16_t color) {
  static char oldstr[9] = "........";      // For compare
  uint8_t i;                               // Index in strings
  uint16_t pos = dsp_getwidth() + TIMEPOS; // X-position of character, TIMEPOS is negative

  if (str[0] == '\0') // Empty string?
  {
    for (i = 0; i < 8; i++) // Set oldstr to dots
    {
      oldstr[i] = '.';
    }
    return; // No actual display yet
  }
  if (tft) // TFT active?
  {
    dsp_setTextColor(color); // Set the requested color
    for (i = 0; i < 8; i++)  // Compare old and new
    {
      if (str[i] != oldstr[i]) // Difference?
      {
        dsp_fillRect(pos, 0, 6, 8, BLACK); // Clear the space for new character
        dsp_setCursor(pos, 0);             // Prepare to show the info
        dsp_print(str[i]);                 // Show the character
        oldstr[i] = str[i];                // Remember for next compare
      }
      pos += 6; // Next position
    }
  }
}
