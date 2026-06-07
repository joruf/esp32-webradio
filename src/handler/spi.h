/**
 * @brief Claim the SPI bus. Uses FreeRTOS semaphores.
 *
 * @param p Identifier string for debugging (e.g., function name or context)
 * @note Waits up to approximately 100 ticks (10 attempts * 10 ticks each)
 * before logging a warning
 */
void claimSPI(const char *p) {
  const TickType_t ctry = 10; // Time to wait for semaphore
  uint32_t count = 0;         // Wait time in ticks

  while (xSemaphoreTake(SPIsem, ctry) != pdTRUE) { // Claim SPI bus
    if (count++ > 10) {
      dbgprint("SPI semaphore not taken within %d ticks by CPU %d, id %s",
               count * ctry, xPortGetCoreID(), p);
    }
  }
}

/**
 * @brief Free the SPI bus. Uses FreeRTOS semaphores.
 */
void releaseSPI() {
  xSemaphoreGive(SPIsem); // Release SPI bus
}