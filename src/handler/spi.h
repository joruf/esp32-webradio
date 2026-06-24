/**
 * @brief Claim the SPI bus. Uses FreeRTOS semaphores.
 *
 * @param p Identifier string for debugging (e.g., function name or context)
 * @return true if the SPI bus was claimed
 */
bool claimSPI(const char *p) {
  const TickType_t ctry = 10; // Time to wait for semaphore
  uint32_t count = 0;         // Wait attempts

  while (xSemaphoreTake(SPIsem, ctry) != pdTRUE) { // Claim SPI bus
    if (++count > 100) {
      dbgprint("SPI semaphore timeout by CPU %d, id %s", xPortGetCoreID(), p);
      return false;
    }
  }

  return true;
}

/**
 * @brief Free the SPI bus. Uses FreeRTOS semaphores.
 */
void releaseSPI() {
  xSemaphoreGive(SPIsem); // Release SPI bus
}
