#ifndef MCP23017_VS1053_H
#define MCP23017_VS1053_H

/**
 * @file mcp23017_vs1053.h
 * @brief MCP23017 GPA0/GPA1 as VS1053 XCS / XDCS on Waveshare S3 Touch 2.8B.
 *
 * Onboard TCA9554 already owns I2C 0x20. Prefer MCP23017 at 0x27 (A0=A1=A2=VCC).
 * Wire SDA=GPIO15, SCL=GPIO7 on the Waveshare I2C header.
 */

#include <Arduino.h>
#include <Wire.h>

#if defined(USE_MCP23017_VS1053)

/** NVS/profile sentinels: not real GPIOs; routed via MCP23017. */
#ifndef VS1053_MCP_CS_SENTINEL
#define VS1053_MCP_CS_SENTINEL ((int8_t)-20)
#endif
#ifndef VS1053_MCP_DCS_SENTINEL
#define VS1053_MCP_DCS_SENTINEL ((int8_t)-21)
#endif

#ifndef MCP23017_VS1053_ADDR
#define MCP23017_VS1053_ADDR 0x27
#endif

namespace mcp23017_vs1053 {
enum : uint8_t {
  kRegIodirA = 0x00,
  kRegGpioA = 0x12,
  kRegOlatA = 0x14,
  kBitCs = 0,  // GPA0 -> VS1053 XCS
  kBitDcs = 1, // GPA1 -> VS1053 XDCS
};

inline bool gReady = false;
inline uint8_t gAddr = MCP23017_VS1053_ADDR;
inline uint8_t gOlatA = 0xFF;

inline bool writeReg_(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(gAddr);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

inline bool readReg_(uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(gAddr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom((int)gAddr, 1) != 1) {
    return false;
  }
  value = (uint8_t)Wire.read();
  return true;
}

inline bool applyOlat_() { return writeReg_(kRegOlatA, gOlatA); }

inline void setBit_(uint8_t bit, bool high) {
  if (high) {
    gOlatA = (uint8_t)(gOlatA | (1u << bit));
  } else {
    gOlatA = (uint8_t)(gOlatA & ~(1u << bit));
  }
}

/** Probe candidate addresses; never use 0x20 (onboard TCA9554). */
inline bool probeAndSelectAddr_() {
  const uint8_t candidates[] = {
      MCP23017_VS1053_ADDR, 0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21,
  };
  for (size_t i = 0; i < sizeof(candidates); i++) {
    const uint8_t addr = candidates[i];
    if (addr == 0x20) {
      continue;
    }
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() != 0) {
      continue;
    }
    gAddr = addr;
    uint8_t iodir = 0;
    if (!readReg_(kRegIodirA, iodir)) {
      continue;
    }
    return true;
  }
  return false;
}

} // namespace mcp23017_vs1053

inline bool vs1053PinIsMcpCs(int8_t pin) { return pin == VS1053_MCP_CS_SENTINEL; }
inline bool vs1053PinIsMcpDcs(int8_t pin) { return pin == VS1053_MCP_DCS_SENTINEL; }
inline bool vs1053UsesMcpCsDcs(int8_t csPin, int8_t dcsPin) {
  return vs1053PinIsMcpCs(csPin) && vs1053PinIsMcpDcs(dcsPin);
}

/**
 * @brief Probe MCP23017 (0x21..0x27) and configure GPA0/GPA1 as idle-high outputs.
 * @return true when expander responds
 */
inline bool mcp23017Vs1053Begin() {
  using namespace mcp23017_vs1053;
  Wire.begin(15, 7);
  Wire.setClock(400000);
  if (!probeAndSelectAddr_()) {
    gReady = false;
    Serial.printf("MCP23017: not found on I2C 15/7 (tried 0x21-0x27, skip 0x20)\n");
    return false;
  }
  // GPA0/GPA1 outputs; leave other port-A bits as inputs (high-Z).
  if (!writeReg_(kRegIodirA, (uint8_t)~((1u << kBitCs) | (1u << kBitDcs)))) {
    gReady = false;
    return false;
  }
  gOlatA = 0xFF; // CS and DCS deselected (HIGH)
  if (!applyOlat_()) {
    gReady = false;
    return false;
  }
  gReady = true;
  Serial.printf("MCP23017: ok at 0x%02X (GPA0=CS GPA1=DCS)\n", gAddr);
  return true;
}

inline bool mcp23017Vs1053Ready() { return mcp23017_vs1053::gReady; }
inline uint8_t mcp23017Vs1053Addr() { return mcp23017_vs1053::gAddr; }

/** Drive XCS: active = LOW. */
inline void mcp23017Vs1053Cs(bool activeLow) {
  using namespace mcp23017_vs1053;
  if (!gReady) {
    return;
  }
  setBit_(kBitCs, !activeLow);
  applyOlat_();
}

/** Drive XDCS: active = LOW. */
inline void mcp23017Vs1053Dcs(bool activeLow) {
  using namespace mcp23017_vs1053;
  if (!gReady) {
    return;
  }
  setBit_(kBitDcs, !activeLow);
  applyOlat_();
}

/** Both chip-selects idle (HIGH). */
inline void mcp23017Vs1053DeselectAll() {
  mcp23017Vs1053Cs(false);
  mcp23017Vs1053Dcs(false);
}

/**
 * @brief Compact hex list of responding I2C addresses on SDA15/SCL7 (for diagnostics).
 */
inline String mcp23017Vs1053ScanI2cHex(void) {
  Wire.begin(15, 7);
  Wire.setClock(100000);
  String out;
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() != 0) {
      continue;
    }
    if (out.length()) {
      out += ',';
    }
    char buf[8];
    snprintf(buf, sizeof(buf), "0x%02X", addr);
    out += buf;
  }
  Wire.setClock(400000);
  return out;
}

#else // !USE_MCP23017_VS1053

inline bool vs1053PinIsMcpCs(int8_t) { return false; }
inline bool vs1053PinIsMcpDcs(int8_t) { return false; }
inline bool vs1053UsesMcpCsDcs(int8_t, int8_t) { return false; }
inline bool mcp23017Vs1053Begin() { return false; }
inline bool mcp23017Vs1053Ready() { return false; }
inline uint8_t mcp23017Vs1053Addr() { return 0; }
inline void mcp23017Vs1053Cs(bool) {}
inline void mcp23017Vs1053Dcs(bool) {}
inline void mcp23017Vs1053DeselectAll() {}
inline String mcp23017Vs1053ScanI2cHex(void) { return String(); }

#endif

#endif
