/**
 * @brief IRAM interrupt service routine for the IR receiver (VS1838B).
 *
 * This function is called on every edge of the IR signal. It measures the
 * time intervals between edges and decodes the IR bits from them.
 *
 * @details
 * - Short pulses (300-800 µs) are decoded as logic "0"
 * - Long pulses (1400-1900 µs) are decoded as logic "1"
 * - After 65 level changes (32 bits + sync), the 32-bit signal is converted
 *   into a 16-bit result and stored in 'ir_value'
 * - The average pulse duration is calculated using a moving average
 *   for both short and long pulses (ir_0, ir_1)
 *
 * @note This function runs in interrupt context (IRAM_ATTR)
 * @note Uses 'sv' (static volatile) for variables that persist across calls
 * @see ir_value (global) - Contains the decoded IR code
 */
void IRAM_ATTR isr_IR() {
  sv uint32_t t0 = 0;          // To get the interval
  sv uint32_t ir_locvalue = 0; // IR code
  sv int ir_loccount = 0;      // Length of code
  uint32_t t1, intval;         // Current time and interval since last change
  uint32_t mask_in = 2;        // Mask input for conversion
  uint16_t mask_out = 1;       // Mask output for conversion

  t1 = micros();                        // Get current time
  intval = t1 - t0;                     // Compute interval
  t0 = t1;                              // Save for next compare
  if ((intval > 300) && (intval < 800)) // Short pulse?
  {
    ir_locvalue = ir_locvalue << 1;              // Shift in a "zero" bit
    ir_loccount++;                               // Count number of received bits
    ir_0 = (ir_0 * 3 + intval) / 4;              // Compute average durartion of a short pulse
  } else if ((intval > 1400) && (intval < 1900)) // Long pulse?
  {
    ir_locvalue = (ir_locvalue << 1) + 1; // Shift in a "one" bit
    ir_loccount++;                        // Count number of received bits
    ir_1 = (ir_1 * 3 + intval) / 4;       // Compute average durartion of a short pulse
  } else if (ir_loccount == 65)           // Value is correct after 65 level changes
  {
    while (mask_in) // Convert 32 bits to 16 bits
    {
      if (ir_locvalue & mask_in) // Bit set in pattern?
      {
        ir_value |= mask_out; // Set set bit in result
      }
      mask_in <<= 2;  // Shift input mask 2 positions
      mask_out <<= 1; // Shift output mask 1 position
    }
    ir_loccount = 0; // Ready for next input
  } else {
    ir_locvalue = 0; // Reset decoding
    ir_loccount = 0;
  }
}

/**
 * @brief IRAM interrupt service routine for rotary encoder rotation.
 *
 * This function is called on every edge of the encoder's CLK signal.
 * It decodes the Manchester-coded signals from the CLK and DT pins.
 *
 * @details
 * - The current state of both pins (CLK, DT) is read as a 2-bit value
 * - An index (0-15) into the 'enc_states[]' table is formed from the
 *   previous and current states
 * - The table returns -1, 0, or +1 for each state change
 * - A full detent step (4 state changes) is detected and 'rotationcount'
 *   is incremented or decremented
 * - 'enc_inactivity' is reset to 0 on every movement
 *
 * @note This function runs in interrupt context (IRAM_ATTR)
 * @note The 'enc_states[]' table must be in DRAM (iram safe) and is declared
 * 'static const'
 * @note No debouncing is needed because the table only considers valid
 * transitions
 * @see rotationcount (global) - Counter for encoder rotations (positive =
 * louder, negative = quieter)
 * @see enc_inactivity (global) - Inactivity counter (reset here)
 */
void IRAM_ATTR isr_enc_turn() {
  sv uint32_t old_state = 0x0001; // Previous state
  sv int16_t locrotcount = 0;     // Local rotation count
  uint8_t act_state = 0;          // The current state of the 2 PINs
  uint8_t inx;                    // Index in enc_state
  sv const int8_t enc_states[] =  // Table must be in DRAM (iram safe)
      {
          0,  // 00 -> 00
          -1, // 00 -> 01                           // dt goes HIGH
          1,  // 00 -> 10
          0,  // 00 -> 11
          1,  // 01 -> 00                           // dt goes LOW
          0,  // 01 -> 01
          0,  // 01 -> 10
          -1, // 01 -> 11                           // clk goes HIGH
          -1, // 10 -> 00                           // clk goes LOW
          0,  // 10 -> 01
          0,  // 10 -> 10
          1,  // 10 -> 11                           // dt goes HIGH
          0,  // 11 -> 00
          1,  // 11 -> 01                           // clk goes LOW
          -1, // 11 -> 10                           // dt goes HIGH
          0   // 11 -> 11
      };
  // Read current state of CLK, DT pin. Result is a 2 bit binary number: 00, 01,
  // 10 or 11.
  act_state = (digitalRead(ini_block.enc_clk_pin) << 1) + digitalRead(ini_block.enc_dt_pin);
  inx = (old_state << 2) + act_state; // Form index in enc_states
  locrotcount += enc_states[inx];     // Get delta: 0, +1 or -1
  if (locrotcount == 4) {
    rotationcount--; // Divide by 4
    locrotcount = 0;
  } else if (locrotcount == -4) // Clockwise=louder
  {
    rotationcount++; // Divide by 4
    locrotcount = 0;
  }
  old_state = act_state; // Remember current status
  enc_inactivity = 0;
}