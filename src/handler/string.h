/**
 * @brief Convert a single character from UTF8 to Extended ASCII.
 * 
 * @param ascii Input byte (UTF8 character byte)
 * @return byte Converted ASCII character, or 0 if the byte should be ignored
 * @note Uses static buffer to handle multi-byte UTF8 sequences
 */
byte utf8ascii(byte ascii) {
  static const byte lut_C3[] = {
    "AAAAAAACEEEEIIIIDNOOOOO#0UUUU###aaaaaaaceeeeiiiidnooooo##uuuuyyy"
  };
  static byte c1 = 0;                    // Last character buffer
  byte res = 0;                          // Result, default 0

  if (ascii <= 0x7F) {                   // Standard ASCII-set 0..0x7F handling
    c1 = 0;
    res = ascii;                         // Return unmodified
  } else {
    switch (c1) {                        // Conversion depending on first UTF8-character
      case 0xC2:
        res = '~';
        break;
      case 0xC3:
        res = lut_C3[ascii - 128];
        break;
      case 0x82:
        if (ascii == 0xAC) {
          res = 'E';                     // Special case Euro symbol
        }
        break;
      default:
        break;
    }
    c1 = ascii;                          // Remember actual character
  }
  return res;                            // Return zero if character should be ignored
}

/**
 * @brief In-place conversion of UTF8 string to Extended ASCII.
 *        The resulting string is shorter than or equal to the original.
 * 
 * @param s Pointer to the null-terminated string to convert (modified in place)
 */
void utf8ascii(char* s) {
  int i, k = 0;                          // Indexes for input and output strings
  char c;

  for (i = 0; s[i]; i++) {               // For every input character
    c = utf8ascii(s[i]);                 // Translate if necessary
    if (c) {                             // Good translation?
      s[k++] = c;                        // Put in output string
    }
  }
  s[k] = 0;                              // Null-terminate the result
}

/**
 * @brief Convert a UTF8 string to Extended ASCII and return as a new String.
 * 
 * @param s Pointer to the null-terminated UTF8 string
 * @return String The converted ASCII string
 */
String utf8ascii(const char* s) {
  int i;
  char c;
  String res = "";                       // Result string

  for (i = 0; s[i]; i++) {               // For every input character
    c = utf8ascii(s[i]);                 // Translate if necessary
    if (c) {                             // Good translation?
      res += String(c);                  // Put in output string
    }
  }
  return res;
}