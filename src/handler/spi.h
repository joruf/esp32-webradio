





//**************************************************************************************************
//                                      C L A I M S P I                                            *
//**************************************************************************************************
// Claim the SPI bus.  Uses FreeRTOS semaphores.                                                   *
//**************************************************************************************************
void claimSPI(const char* p )
{
  const        TickType_t ctry = 10 ;                       // Time to wait for semaphore
  uint32_t     count = 0 ;                                  // Wait time in ticks

  while(xSemaphoreTake(SPIsem, ctry)!= pdTRUE )     // Claim SPI bus
  {
    if(count++ > 10 )
    {
      dbgprint("SPI semaphore not taken within %d ticks by CPU %d, id %s",
                 count * ctry,
                 xPortGetCoreID(),
                 p);
    }
  }
}


//**************************************************************************************************
//                                   R E L E A S E S P I                                           *
//**************************************************************************************************
// Free the SPI bus.  Uses FreeRTOS semaphores.                                                    *
//**************************************************************************************************
void releaseSPI() {
  xSemaphoreGive(SPIsem);                            // Release SPI bus
}
