/**
 * @brief Extracts a field from a delimited string.
 *
 * Searches for the 'index'-th occurrence of 'separator' in 'data' and
 * returns the substring between that delimiter and the next one.
 *
 * @param data      String to search through
 * @param separator Delimiter character (e.g. ':', ',', ';')
 * @param index     Desired field position (0 = first field)
 * @return String   The extracted field, or empty string if index does not exist
 */
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

/**
 * @brief Helper function: Swaps two entries in the nvskeys array.
 *
 * @param a Index of the first entry
 * @param b Index of the second entry
 * @param tmpstr Temporary buffer (at least 16 bytes) for the swap
 */
static void swapKeys(uint16_t a, uint16_t b, char *tmpstr) {
  strcpy(tmpstr, nvskeys[a]);
  strcpy(nvskeys[a], nvskeys[b]);
  strcpy(nvskeys[b], tmpstr);
}

/**
 * @brief Performs bubble sort on the global nvskeys array.
 *
 * Sorts the first n entries of the array in ascending alphabetical order.
 *
 * @param n Number of elements to sort (maximum array size)
 * @pre nvskeys must be initialized and contain at least n valid strings
 */
void bubbleSortKeys(uint16_t n) {
  uint16_t i, j;   // Indexes in nvskeys
  char tmpstr[16]; // Temp. storage for a key

  for (i = 0; i < n - 1; i++) // Examine all keys
  {
    for (j = 0; j < n - i - 1; j++) // Compare to following keys
    {
      if (strcmp(nvskeys[j], nvskeys[j + 1]) > 0) // Next key out of order?
      {
        swapKeys(j, j + 1, tmpstr);
      }
    }
  }
}

/**
 * @brief Reads the ADC value and performs a moving average filter.
 *
 * Uses an IIR filter (moving average) with a factor of 15/16.
 * The new value is calculated as (15 * old_value + new_raw_value) / 16.
 *
 * @param raw_value The raw ADC measurement value (0-4095 for 12-bit ADC)
 * @return uint32_t The updated, filtered ADC value
 * @note The filtered value is stored in the global 'adcval' variable
 */
static uint32_t filterADCValue(uint32_t raw_value) { return (15 * adcval + raw_value) / 16; }

/**
 * @brief TFT display and ADC measurement task.
 *
 * This task runs with low priority in an infinite loop.
 * It handles:
 * - TFT updates (via handleRenderData)
 * - ADC measurement and filtering (volume control, etc.)
 *
 * @param parameter Unused parameter (for FreeRTOS task creation)
 * @note The task is never terminated (vTaskDelete is never reached)
 */
void spftask(void *parameter) {
  while (true) {
    handleRenderData();                  // Maybe some special funcs?
    vTaskDelay(100 / portTICK_PERIOD_MS); // Pause for a short time

    uint32_t raw_adc = adc1_get_raw(ADC1_CHANNEL_0); // Read ADC
    adcval = filterADCValue(raw_adc);                // Do some filtering
  }
  // vTaskDelete(NULL);                                                       //
  // Will never arrive here
}