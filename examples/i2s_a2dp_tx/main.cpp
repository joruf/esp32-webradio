/**
 * ESP32-S3 I2S master TX: continuous stereo PCM tone for the A2DP bridge.
 *
 * Pair with env esp32_a2dp_source on a Classic-BT WROOM (I2S slave RX).
 * Serial logs via USB CDC only (GPIO43/44 are I2S clocks, not UART0).
 *
 * Serial (115200 USB CDC): status | help
 */

#include <Arduino.h>
#include <cmath>
#include <cstring>
#include <driver/i2s_std.h>

#include "i2s_a2dp_tx_config.h"

static i2s_chan_handle_t gI2sTx = nullptr;
static bool gI2sReady = false;
static uint32_t gTonePhase = 0;
static uint64_t gBytesWritten = 0;
static uint32_t gWriteErrors = 0;

static bool i2sInitMasterTx() {
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  chan_cfg.auto_clear = true;
  if (i2s_new_channel(&chan_cfg, &gI2sTx, nullptr) != ESP_OK) {
    Serial.println("I2S: new_channel failed");
    return false;
  }

  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_TX_SAMPLE_RATE),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                      I2S_SLOT_MODE_STEREO),
      .gpio_cfg =
          {
              .mclk = I2S_GPIO_UNUSED,
              .bclk = (gpio_num_t)I2S_TX_PIN_BCLK,
              .ws = (gpio_num_t)I2S_TX_PIN_LRCK,
              .dout = (gpio_num_t)I2S_TX_PIN_DOUT,
              .din = I2S_GPIO_UNUSED,
              .invert_flags =
                  {
                      .mclk_inv = false,
                      .bclk_inv = false,
                      .ws_inv = false,
                  },
          },
  };

  if (i2s_channel_init_std_mode(gI2sTx, &std_cfg) != ESP_OK) {
    Serial.println("I2S: init_std_mode failed");
    return false;
  }
  if (i2s_channel_enable(gI2sTx) != ESP_OK) {
    Serial.println("I2S: enable failed");
    return false;
  }

  Serial.printf("I2S: master TX ready BCLK=%d LRCK=%d DOUT=%d @ %d Hz, tone %d Hz\n",
                I2S_TX_PIN_BCLK, I2S_TX_PIN_LRCK, I2S_TX_PIN_DOUT, I2S_TX_SAMPLE_RATE,
                I2S_TX_TONE_HZ);
  Serial.println("Note: GPIO43/44 = UART0; use USB CDC for this serial console.");
  return true;
}

static void fillToneStereo(int16_t *samples, size_t stereoFrames) {
  const float twoPi = 6.28318530718f;
  const float step = twoPi * (float)I2S_TX_TONE_HZ / (float)I2S_TX_SAMPLE_RATE;
  const float amp = (float)I2S_TX_AMPLITUDE;
  for (size_t i = 0; i < stereoFrames; ++i) {
    const float s = sinf((float)gTonePhase * step);
    const int16_t v = (int16_t)(s * amp);
    samples[2 * i] = v;
    samples[2 * i + 1] = v;
    gTonePhase++;
  }
}

static void printHelp() {
  Serial.println("Commands: status | help");
}

static void printStatus() {
  Serial.println("---- S3 I2S A2DP TX status ----");
  Serial.printf("  I2S ready: %s\n", gI2sReady ? "yes" : "no");
  Serial.printf("  pins BCLK=%d LRCK=%d DOUT=%d\n", I2S_TX_PIN_BCLK, I2S_TX_PIN_LRCK,
                I2S_TX_PIN_DOUT);
  Serial.printf("  sample rate: %d  tone: %d Hz\n", I2S_TX_SAMPLE_RATE, I2S_TX_TONE_HZ);
  Serial.printf("  bytes written: %llu  write errors: %lu\n",
                (unsigned long long)gBytesWritten, (unsigned long)gWriteErrors);
  Serial.println("  WROOM expect: BCLK=26 LRCK=25 DIN=32, GND common");
  Serial.println("  Serial: USB CDC only (UART0 pins used for I2S)");
  Serial.println("--------------------------------");
}

static void handleSerialLine(char *line) {
  while (*line == ' ' || *line == '\t') {
    ++line;
  }
  if (*line == '\0') {
    return;
  }
  if (strcmp(line, "help") == 0 || strcmp(line, "?") == 0) {
    printHelp();
    return;
  }
  if (strcmp(line, "status") == 0) {
    printStatus();
    return;
  }
  Serial.printf("Unknown: '%s' (try help)\n", line);
}

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println();
  Serial.println("ESP32-S3 I2S master TX (A2DP bridge tone)");
  Serial.println("PCM -> WROOM I2S slave -> Classic BT headphones");
  printHelp();

  gI2sReady = i2sInitMasterTx();
  printStatus();
}

void loop() {
  static int16_t buf[256 * 2]; /* 256 stereo frames */
  static char line[96];
  static size_t len = 0;
  static uint32_t lastBeat = 0;

  while (Serial.available() > 0) {
    const char c = (char)Serial.read();
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      line[len] = '\0';
      handleSerialLine(line);
      len = 0;
      continue;
    }
    if (len + 1 < sizeof(line)) {
      line[len++] = c;
    }
  }

  if (gI2sReady && gI2sTx) {
    fillToneStereo(buf, 256);
    size_t written = 0;
    const esp_err_t err =
        i2s_channel_write(gI2sTx, buf, sizeof(buf), &written, pdMS_TO_TICKS(100));
    if (err == ESP_OK) {
      gBytesWritten += written;
    } else {
      gWriteErrors++;
    }
  } else {
    delay(50);
  }

  const uint32_t now = millis();
  if (now - lastBeat >= 5000) {
    lastBeat = now;
    Serial.printf("beat: ready=%d bytes=%llu err=%lu tone=%dHz\n", (int)gI2sReady,
                  (unsigned long long)gBytesWritten, (unsigned long)gWriteErrors,
                  I2S_TX_TONE_HZ);
  }
}
