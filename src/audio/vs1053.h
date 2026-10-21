#include "config/gpio_valid.h"
#include "audio/mcp23017_vs1053.h"
#include "boot/st7701_boot_wdt.h"
#include "audio/vs1053_fault.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//
//**************************************************************************************************
// VS1053 stuff.  Based on maniacbug library. *
//**************************************************************************************************
// VS1053 class definition. *
//**************************************************************************************************
class VS1053 {
private:
  int8_t cs_pin;        // Pin where CS line is connected
  int8_t dcs_pin;       // Pin where DCS line is connected
  int8_t dreq_pin;      // Pin where DREQ line is connected
  int8_t shutdown_pin;  // Pin where the shutdown line is connected
  int8_t shutdownx_pin; // Pin where the shutdown (inversed) line is connected
  uint8_t curvol = 0xFF; // Current volume setting 0..100%, force first setVolume write
  const uint8_t vs1053_chunk_size = 32;
  // SCI Register
  const uint8_t SCI_MODE = 0x0;
  const uint8_t SCI_BASS = 0x2;
  const uint8_t SCI_CLOCKF = 0x3;
  const uint8_t SCI_AUDATA = 0x5;
  const uint8_t SCI_WRAM = 0x6;
  const uint8_t SCI_WRAMADDR = 0x7;
  const uint8_t SCI_AIADDR = 0xA;
  const uint8_t SCI_VOL = 0xB;
  const uint8_t SCI_AICTRL0 = 0xC;
  const uint8_t SCI_AICTRL1 = 0xD;
  const uint8_t SCI_num_registers = 0xF;
  // SCI_MODE bits
  const uint8_t SM_SDINEW = 11; // Bitnumber in SCI_MODE always on
  const uint8_t SM_RESET = 2;   // Bitnumber in SCI_MODE soft reset
  const uint8_t SM_CANCEL = 3;  // Bitnumber in SCI_MODE cancel song
  const uint8_t SM_TESTS = 5;   // Bitnumber in SCI_MODE for tests
  const uint8_t SM_LINE1 = 14;  // Bitnumber in SCI_MODE for Line input
  SPISettings VS1053_SPI;       // SPI settings for this slave
  uint8_t endFillByte;          // Byte to send when stopping song
  bool okay = false;            // VS1053 is working (set true only after begin/testComm)
protected:
  inline bool await_data_request() const {
    if (dreq_pin < 0) {
      return true;
    }
    // Timeout so a missing VS1053 (DREQ stuck low) cannot hang stream tasks.
    uint32_t start = millis();
    while (!digitalRead(dreq_pin)) {
      if ((millis() - start) > 500) {
        vs1053RecordDreqTimeout();
        return false;
      }
      st7701FeedTaskWdt();
      vTaskDelay(1);
    }
    return true;
  }

  inline void control_mode_on() const {
    if (vs1053UsesMcpCsDcs(cs_pin, dcs_pin)) {
      if (!mcp23017Vs1053Ready()) {
        return;
      }
      SPI.beginTransaction(VS1053_SPI);
      mcp23017Vs1053Dcs(false);
      mcp23017Vs1053Cs(true);
      return;
    }
    if (cs_pin < 0 || dcs_pin < 0) {
      return;
    }
    SPI.beginTransaction(VS1053_SPI); // Prevent other SPI users
    digitalWrite(cs_pin, LOW);
  }

  inline void control_mode_off() const {
    if (vs1053UsesMcpCsDcs(cs_pin, dcs_pin)) {
      mcp23017Vs1053Cs(false);
      SPI.endTransaction();
      return;
    }
    if (cs_pin < 0 || dcs_pin < 0) {
      return;
    }
    digitalWrite(cs_pin, HIGH); // End control mode
    SPI.endTransaction();       // Allow other SPI users
  }

  inline void data_mode_on() const {
    if (vs1053UsesMcpCsDcs(cs_pin, dcs_pin)) {
      if (!mcp23017Vs1053Ready()) {
        return;
      }
      SPI.beginTransaction(VS1053_SPI);
      mcp23017Vs1053Cs(false);
      mcp23017Vs1053Dcs(true);
      return;
    }
    if (cs_pin < 0 || dcs_pin < 0) {
      return;
    }
    SPI.beginTransaction(VS1053_SPI); // Prevent other SPI users
    // digitalWrite(cs_pin, HIGH);           // Bring slave in data mode
    digitalWrite(dcs_pin, LOW);
  }

  inline void data_mode_off() const {
    if (vs1053UsesMcpCsDcs(cs_pin, dcs_pin)) {
      mcp23017Vs1053Dcs(false);
      SPI.endTransaction();
      return;
    }
    if (cs_pin < 0 || dcs_pin < 0) {
      return;
    }
    digitalWrite(dcs_pin, HIGH); // End data mode
    SPI.endTransaction();        // Allow other SPI users
  }

  void write_register(uint8_t _reg, uint16_t _value) const;
  inline bool sdi_send_buffer(uint8_t *data, size_t len);
  void sdi_send_fillers(size_t length);
  void wram_write(uint16_t address, uint16_t data);
  uint16_t wram_read(uint16_t address);
  void output_enable(bool ena); // Enable amplifier through shutdown pin(s)

public:
  // Constructor.  Only sets pin values.  Doesn't touch the chip.  Be sure to
  // call begin()!
  VS1053(int8_t _cs_pin, int8_t _dcs_pin, int8_t _dreq_pin, int8_t _shutdown_pin, int8_t _shutdownx_pin);
  uint16_t read_register(uint8_t _reg) const;
  void begin(); // Begin operation.  Sets pins correctly,
  // and prepares SPI bus.
  void startSong(); // Prepare to start playing. Call this each
  // time a new song starts.
  inline bool playChunk(uint8_t *data, // Play a chunk of data.  Copies the data to
                        size_t len);   // the chip.  Blocks until complete.
  // Returns true if more data can be added
  // to fifo
  void stopSong(); // Finish playing a song. Call this after
  // the last playChunk call.
  void setVolume(uint8_t vol); // Set the player volume.Level from 0-100,
  // higher is louder.
  void setOutputEnabled(bool ena) { output_enable(ena); }
  void setTone(uint8_t *rtone); // Set the player baas/treble, 4 nibbles for
  // treble gain/freq and bass gain/freq
  inline uint8_t getVolume() const // Get the current volume setting.
  {                                // higher is louder.
    return curvol;
  }
  bool isReady() const { return okay; }
  /** Update CS/DCS/DREQ (e.g. MCP missing → GPIO fallback) before begin(). */
  void setPins(int8_t _cs_pin, int8_t _dcs_pin, int8_t _dreq_pin) {
    cs_pin = _cs_pin;
    dcs_pin = _dcs_pin;
    dreq_pin = _dreq_pin;
    okay = false;
  }
  void printDetails(const char *header); // Print config details to serial output
  void softReset();                      // Do a soft reset
  bool testComm(const char *header);     // Test communication with module
  inline bool data_request() const {
    if (dreq_pin < 0) {
      return true;
    }
    return (digitalRead(dreq_pin) == HIGH);
  }
  void AdjustRate(long ppm2); // Fine tune the datarate
  /** Start VS1053 hardware sine test; n selects frequency (datasheet table). */
  void startSineTest(uint8_t n);
  /** Stop sine test and clear SM_TESTS. */
  void stopSineTest();
};

//**************************************************************************************************
// VS1053 class implementation. *
//**************************************************************************************************

VS1053::VS1053(int8_t _cs_pin, int8_t _dcs_pin, int8_t _dreq_pin, int8_t _shutdown_pin, int8_t _shutdownx_pin)
    : cs_pin(_cs_pin), dcs_pin(_dcs_pin), dreq_pin(_dreq_pin), shutdown_pin(_shutdown_pin),
      shutdownx_pin(_shutdownx_pin) {}

uint16_t VS1053::read_register(uint8_t _reg) const {
  uint16_t result;

  if ((!vs1053UsesMcpCsDcs(cs_pin, dcs_pin) && (cs_pin < 0 || dcs_pin < 0)) || dreq_pin < 0) {
    return 0xFFFF;
  }
  control_mode_on();
  SPI.write(3);    // Read operation
  SPI.write(_reg); // Register to write (0..0xF)
  // Note: transfer16 does not seem to work
  result = (SPI.transfer(0xFF) << 8) | // Read 16 bits data
           (SPI.transfer(0xFF));
  await_data_request(); // Wait for DREQ to be HIGH again
  control_mode_off();
  return result;
}

void VS1053::write_register(uint8_t _reg, uint16_t _value) const {
  if ((!vs1053UsesMcpCsDcs(cs_pin, dcs_pin) && (cs_pin < 0 || dcs_pin < 0)) || dreq_pin < 0) {
    return;
  }
  control_mode_on();
  SPI.write(2);        // Write operation
  SPI.write(_reg);     // Register to write (0..0xF)
  SPI.write16(_value); // Send 16 bits data
  await_data_request();
  control_mode_off();
}

bool VS1053::sdi_send_buffer(uint8_t *data, size_t len) {
  size_t chunk_length; // Length of chunk 32 byte or shorter

  data_mode_on();
  while (len) // More to do?
  {
    chunk_length = len;
    if (len > vs1053_chunk_size) {
      chunk_length = vs1053_chunk_size;
    }
    len -= chunk_length;
    if (!await_data_request()) {
      data_mode_off();
      return false;
    }
    SPI.writeBytes(data, chunk_length);
    data += chunk_length;
  }
  data_mode_off();
  if (data_request()) {
    vs1053NoteSuccessfulFeed();
    return true;
  }
  return false;
}

void VS1053::sdi_send_fillers(size_t len) {
  size_t chunk_length; // Length of chunk 32 byte or shorter

  data_mode_on();
  while (len) // More to do?
  {
    if (!await_data_request()) {
      data_mode_off();
      return;
    }
    chunk_length = len;
    if (len > vs1053_chunk_size) {
      chunk_length = vs1053_chunk_size;
    }
    len -= chunk_length;
    while (chunk_length--) {
      SPI.write(endFillByte);
    }
  }
  data_mode_off();
}

void VS1053::wram_write(uint16_t address, uint16_t data) {
  write_register(SCI_WRAMADDR, address);
  write_register(SCI_WRAM, data);
}

uint16_t VS1053::wram_read(uint16_t address) {
  write_register(SCI_WRAMADDR, address); // Start reading from WRAM
  return read_register(SCI_WRAM);        // Read back result
}

bool VS1053::testComm(const char *header) {
  // Test the communication with the VS1053 module.  The result wille be
  // returned. If DREQ is low, there is problably no VS1053 connected.  Pull the
  // line HIGH in order to prevent an endless loop waiting for this signal.  The
  // rest of the software will still work, but readbacks from VS1053 will fail.
  int i; // Loop control
  uint16_t r1, r2, cnt = 0;
  uint16_t delta = 300; // 3 for fast SPI

  dbgprint(header); // Show a header
  if (dreq_pin >= 0) {
    uint32_t dreqWaitStart = millis();
    while (!digitalRead(dreq_pin) && (millis() - dreqWaitStart) < 1200) {
      delay(2);
      st7701FeedTaskWdt();
    }
  }
  if (dreq_pin >= 0 && !digitalRead(dreq_pin)) {
    dbgprint("VS1053 DREQ stuck low (CS=%d DCS=%d DREQ=%d) - check wiring", cs_pin, dcs_pin, dreq_pin);
    pinMode(dreq_pin, INPUT_PULLUP);
    okay = false;
    return false;
  }
  // Further TESTING.  Check if SCI bus can write and read without errors.
  // We will use the volume setting for this.
  // Will give warnings on serial output if DEBUG is active.
  // A maximum of 20 errors will be reported.
  if (strstr(header, "Fast")) {
    delta = 3; // Fast SPI, more loops
  }
  const int maxSweep = strstr(header, "Fast") ? 4000 : 3500;
  for (i = 0; (i < maxSweep) && (cnt < 20); i += delta) {
    write_register(SCI_VOL, i);  // Write data to SCI_VOL
    r1 = read_register(SCI_VOL); // Read back for the first time
    r2 = read_register(SCI_VOL); // Read back a second time
    if (r1 != r2 || i != r1 || i != r2) {
      dbgprint("VS1053 SPI error. SB:%04X R1:%04X R2:%04X", i, r1, r2);
      cnt++;
      delay(10);
    }
    if ((i % (delta * 8)) == 0) {
      st7701FeedTaskWdt();
    }
  }
  okay = (cnt == 0); // True if working correctly
  return (okay);     // Return the result
}

void VS1053::begin() {
  if (!vs1053SpiBusConfigured(cs_pin, dcs_pin, dreq_pin, ini_block.spi_sck_pin, ini_block.spi_miso_pin,
                              ini_block.spi_mosi_pin)) {
    dbgprint("VS1053 pins incomplete or unsafe (CS/DCS/DREQ/SPI), skipping audio decoder");
    okay = false;
    return;
  }
  pinMode(dreq_pin, INPUT); // DREQ is an input
  if (vs1053UsesMcpCsDcs(cs_pin, dcs_pin)) {
    if (!mcp23017Vs1053Ready() && !mcp23017Vs1053Begin()) {
      dbgprint("VS1053 MCP23017 CS/DCS unavailable, skipping audio decoder");
      okay = false;
      return;
    }
    mcp23017Vs1053DeselectAll();
  } else {
    pinMode(cs_pin, OUTPUT);  // The SCI and SDI signals
    pinMode(dcs_pin, OUTPUT);
    digitalWrite(dcs_pin, HIGH); // Start HIGH for SCI en SDI
    digitalWrite(cs_pin, HIGH);
  }
  if (shutdown_pin >= 0) // Shutdown or VS1053 XRESET (active low)
  {
    pinMode(shutdown_pin, OUTPUT);
    digitalWrite(shutdown_pin, HIGH);
    delay(10);
    digitalWrite(shutdown_pin, LOW);
    delay(10);
    digitalWrite(shutdown_pin, HIGH);
    delay(10);
  }
  if (shutdownx_pin >= 0) // Shutdown (inversed logic) in use?
  {
    pinMode(shutdownx_pin, OUTPUT);
  }
  output_enable(false); // Disable amplifier through shutdown pin(s)
  delay(100);
  st7701FeedTaskWdt();
  // Init SPI in slow mode(0.2 MHz)
  VS1053_SPI = SPISettings(200000, MSBFIRST, SPI_MODE0);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  delay(20);

  const bool writeOnly = (ini_block.spi_miso_pin < 0);
  bool commOk = false;
  if (writeOnly) {
    // No MISO (S3 pin shortage): rely on DREQ handshake after SCI writes.
    dbgprint("VS1053 write-only bring-up (no MISO), DREQ=%d", (int)dreq_pin);
    if (dreq_pin >= 0 && !digitalRead(dreq_pin)) {
      uint32_t t0 = millis();
      while (!digitalRead(dreq_pin) && (millis() - t0) < 1200) {
        delay(2);
        st7701FeedTaskWdt();
      }
    }
    commOk = (dreq_pin < 0) || digitalRead(dreq_pin);
  } else {
    commOk = testComm("Slow SPI, Testing VS1053 read/write registers...");
  }

  if (commOk) {
    // Most VS1053 modules will start up in midi mode.  The result is that there
    // is no audio when playing MP3.  You can modify the board, but there is a
    // more elegant way:
    wram_write(0xC017, 3); // GPIO DDR = 3
    wram_write(0xC019, 0); // GPIO ODATA = 0
    delay(100);
    softReset(); // Do a soft reset
    write_register(SCI_AUDATA, 44100 + 1); // 44.1kHz + stereo
    write_register(SCI_CLOCKF, 6 << 12);   // Normal clock settings
    VS1053_SPI = SPISettings(5000000, MSBFIRST, SPI_MODE0);
    // Headphone / DAC path (avoid LINE1-only mode which can mute the jack on some boards).
    write_register(SCI_MODE, _BV(SM_SDINEW));
    // Unmute fully once after reset (0x0000 = loudest on VS1053).
    write_register(SCI_VOL, 0x0000);
    curvol = 0xFF; // force next setVolume() to re-apply mapped level

    if (writeOnly) {
      endFillByte = 0x00;
      // Require DREQ high after SCI traffic; still best-effort without MISO.
      okay = (dreq_pin < 0) || digitalRead(dreq_pin);
      dbgprint("VS1053 write-only init %s (DREQ=%d)", okay ? "OK" : "FAIL",
               (dreq_pin >= 0 && digitalRead(dreq_pin)) ? 1 : 0);
    } else {
      testComm("Fast SPI, Testing VS1053 read/write registers again...");
      delay(10);
      await_data_request();
      endFillByte = wram_read(0x1E06) & 0xFF;
      dbgprint("endFillByte is %X", endFillByte);
    }
    delay(100);
  } else {
    okay = false;
  }
}

void VS1053::setVolume(uint8_t vol) {
  // Set volume.  Both left and right.
  // Input value is 0..100.  100 is the loudest.
  // Clicking reduced by using 0xf8 to 0x00 as limits.
  uint16_t value; // Value to send to SCI_VOL

  if (vol != curvol) {
    curvol = vol;                         // Save for later use
    if (!okay) {
      return;
    }
    value = map(vol, 0, 100, 0xF8, 0x00); // 0..100% to one channel
    value = (value << 8) | value;
    write_register(SCI_VOL, value); // Volume left and right
  }
}

void VS1053::setTone(uint8_t *rtone) { // Set bass/treble (4 nibbles)
  if (!okay) {
    return;
  }
  // Set tone characteristics.  See documentation for the 4 nibbles.
  uint16_t value = 0; // Value to send to SCI_BASS
  int i;              // Loop control

  for (i = 0; i < 4; i++) {
    value = (value << 4) | rtone[i]; // Shift next nibble in
  }
  write_register(SCI_BASS, value); // Volume left and right
}

void VS1053::startSong() {
  if (!okay) {
    return;
  }
  sdi_send_fillers(10);
  // Amplifier stays muted here; it is enabled by setVolume() when fade-in starts.
}

bool VS1053::playChunk(uint8_t *data, size_t len) {
  return okay && sdi_send_buffer(data, len); // True if more data can be added to fifo
}

void VS1053::stopSong() {
  if (!okay) {
    return;
  }
  uint16_t modereg; // Read from mode register
  int i;            // Loop control

  sdi_send_fillers(2052);
  output_enable(false); // Disable amplifier through shutdown pin(s)
  delay(10);
  write_register(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_CANCEL));
  for (i = 0; i < 80; i++) {
    sdi_send_fillers(32);
    modereg = read_register(SCI_MODE); // Read status
    if ((modereg & _BV(SM_CANCEL)) == 0) {
      sdi_send_fillers(2052);
      // dbgprint("Song stopped correctly after %d msec", i * 10);
      return;
    }
    st7701FeedTaskWdt();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  printDetails("Song stopped incorrectly!");
}

void VS1053::softReset() {
  write_register(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_RESET));
  delay(10);
  await_data_request();
}

void VS1053::startSineTest(uint8_t n) {
  uint16_t mode = read_register(SCI_MODE);
  write_register(SCI_MODE, mode | _BV(SM_TESTS));
  uint8_t start[8] = {0x53, 0xEF, 0x6E, n, 0x00, 0x00, 0x00, 0x00};
  playChunk(start, sizeof(start));
}

void VS1053::stopSineTest() {
  uint8_t end[8] = {0x45, 0x78, 0x69, 0x74, 0x00, 0x00, 0x00, 0x00};
  playChunk(end, sizeof(end));
  uint16_t mode = read_register(SCI_MODE);
  write_register(SCI_MODE, (uint16_t)((mode & ~_BV(SM_TESTS)) | _BV(SM_SDINEW)));
}

void VS1053::printDetails(const char *header) {
  uint16_t regbuf[16];
  uint8_t i;

  dbgprint(header);
  dbgprint("REG   Contents");
  dbgprint("---   -----");
  for (i = 0; i <= SCI_num_registers; i++) {
    regbuf[i] = read_register(i);
  }
  for (i = 0; i <= SCI_num_registers; i++) {
    delay(5);
    dbgprint("%3X - %5X", i, regbuf[i]);
  }
}

void VS1053::output_enable(bool ena) { // Enable amplifier through shutdown pin(s)
  if (shutdown_pin >= 0) {
    if (shutdownx_pin >= 0) {
      digitalWrite(shutdown_pin, !ena);
    } else {
      // pin_shutdown wired to VS1053 XRESET (idle high), not an amp mute line
      digitalWrite(shutdown_pin, HIGH);
    }
  }
  if (shutdownx_pin >= 0) // Shutdown (inversed logic) in use?
  {
    digitalWrite(shutdownx_pin, ena); // Shut down or enable audio output
  }
}

void VS1053::AdjustRate(long ppm2) { // Fine tune the data rate
  if (!okay) {
    return;
  }
  write_register(SCI_WRAMADDR, 0x1e07);
  write_register(SCI_WRAM, ppm2);
  write_register(SCI_WRAM, ppm2 >> 16);
  // oldClock4KHz = 0 forces  adjustment calculation when rate checked.
  write_register(SCI_WRAMADDR, 0x5b1c);
  write_register(SCI_WRAM, 0);
  // Write to AUDATA or CLOCKF checks rate and recalculates adjustment.
  write_register(SCI_AUDATA, read_register(SCI_AUDATA));
}
