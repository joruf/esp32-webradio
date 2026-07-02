#ifndef ARDUINO_STUB_H
#define ARDUINO_STUB_H

/**
 * @file Arduino.h
 * @brief Minimal Arduino stub for host-based (native) unit tests.
 *
 * Only provides the primitive types and helpers used by the hardware-independent
 * headers under test. It is never compiled into the firmware build.
 */

#include <cstdint>
#include <cstring>
#include <algorithm>

#ifndef constrain
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif

#endif /* ARDUINO_STUB_H */
