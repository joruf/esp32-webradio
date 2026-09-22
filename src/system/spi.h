/**
 * @file spi.h
 * @brief Claim/release helpers for the shared SPI bus.
 */

#ifndef SYSTEM_SPI_H
#define SYSTEM_SPI_H

/**
 * @brief Claim the SPI bus. Uses FreeRTOS semaphores.
 *
 * @param p Identifier string for debugging (e.g., function name or context)
 * @return true if the SPI bus was claimed
 */
inline bool claimSPI(const char *p) {
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
inline void releaseSPI() {
  xSemaphoreGive(SPIsem); // Release SPI bus
}

#endif
