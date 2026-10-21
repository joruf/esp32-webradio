/**
 * ESP32-WROOM Classic Bluetooth A2DP Source firmware.
 *
 * I2S slave RX (legacy driver) <- S3 I2S master TX.
 * Forwards PCM to Classic Bluetooth A2DP Source (Linux / headphones).
 *
 * Serial (115200): status | vol <0-127> | tone | i2s | probe | help
 */

#include <Arduino.h>
#include <BluetoothA2DPSource.h>
#include <cmath>
#include <cstring>

#include "a2dp_bridge_config.h"
#include "esp_task_wdt.h"

#if A2DP_USE_I2S_INPUT
#include <driver/i2s.h>
#include <driver/gpio.h>
#if defined(CONFIG_IDF_TARGET_ESP32)
#include <driver/dac.h>
#endif
#endif

static BluetoothA2DPSource gA2dp;
static volatile uint8_t gVolume = A2DP_VOLUME;
static volatile bool gForceTone = false;
static volatile bool gBtConnected = false;
static char gLastSink[64] = {0};
static uint32_t gTonePhase = 0;
static uint32_t gI2sOkFrames = 0;
static uint32_t gToneFrames = 0;
static bool gI2sReady = false;

#if A2DP_USE_I2S_INPUT
/* Legacy I2S slave RX: new std-driver slave never filled DMA on this chip. */
static const i2s_port_t kI2sPort = I2S_NUM_0;

static void i2sShutdownRx() {
  if (gI2sReady) {
    i2s_driver_uninstall(kI2sPort);
  }
  gI2sReady = false;
}

static bool i2sInitSlaveRx() {
  i2sShutdownRx();

#if defined(CONFIG_IDF_TARGET_ESP32)
  /* GPIO25/26 are DAC1/DAC2; reset pads before claiming for I2S. */
  dac_output_disable(DAC_CHANNEL_1);
  dac_output_disable(DAC_CHANNEL_2);
  gpio_reset_pin((gpio_num_t)I2S_PIN_BCLK);
  gpio_reset_pin((gpio_num_t)I2S_PIN_LRCK);
  gpio_reset_pin((gpio_num_t)I2S_PIN_DIN);
#endif

  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_SLAVE | I2S_MODE_RX);
  cfg.sample_rate = A2DP_SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 256;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = false;
  cfg.fixed_mclk = 0;

  if (i2s_driver_install(kI2sPort, &cfg, 0, nullptr) != ESP_OK) {
    Serial.println("I2S: driver_install failed");
    return false;
  }

  i2s_pin_config_t pins = {};
  pins.bck_io_num = I2S_PIN_BCLK;
  pins.ws_io_num = I2S_PIN_LRCK;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = I2S_PIN_DIN;
  if (i2s_set_pin(kI2sPort, &pins) != ESP_OK) {
    Serial.println("I2S: set_pin failed");
    i2s_driver_uninstall(kI2sPort);
    return false;
  }
  (void)i2s_zero_dma_buffer(kI2sPort);

  gI2sReady = true;
  Serial.printf("I2S: SLAVE RX BCLK=%d LRCK=%d DIN=%d @ %d Hz (S3 must be I2S master TX)\n",
                I2S_PIN_BCLK, I2S_PIN_LRCK, I2S_PIN_DIN, A2DP_SAMPLE_RATE);
  return true;
}

static int32_t readI2sPcm(uint8_t *data, int32_t byteCount) {
  if (!gI2sReady || byteCount <= 0) {
    return 0;
  }
  size_t got = 0;
  const esp_err_t err = i2s_read(kI2sPort, data, (size_t)byteCount, &got, pdMS_TO_TICKS(20));
  if (err != ESP_OK && err != ESP_ERR_TIMEOUT) {
    return 0;
  }
  return (int32_t)got;
}
#endif

static void fillToneFrames(Frame *frame, int32_t frame_count) {
  const float step = 6.28318530718f * (float)A2DP_TONE_HZ / (float)A2DP_SAMPLE_RATE;
  const float amp = 12000.0f * ((float)gVolume / 127.0f);
  static float phase = 0.0f;
  for (int32_t i = 0; i < frame_count; ++i) {
    float x = phase;
    if (x > 3.14159265f) {
      x -= 6.28318530718f;
      phase -= 6.28318530718f;
    }
    const float x2 = x * x;
    const float s = x * (1.0f - x2 * (1.0f / 6.0f - x2 * (1.0f / 120.0f)));
    const int16_t v = (int16_t)(s * amp);
    frame[i].channel1 = v;
    frame[i].channel2 = v;
    phase += step;
    gTonePhase++;
  }
}

static int32_t getDataFrames(Frame *frame, int32_t frame_count) {
  if (frame_count <= 0 || frame == nullptr) {
    return 0;
  }

  if (gForceTone) {
    fillToneFrames(frame, frame_count);
    gToneFrames += (uint32_t)frame_count;
    return frame_count;
  }

#if A2DP_USE_I2S_INPUT
  if (gI2sReady) {
    const int32_t need = frame_count * (int32_t)sizeof(Frame);
    const int32_t got = readI2sPcm(reinterpret_cast<uint8_t *>(frame), need);
    if (got >= need) {
      gI2sOkFrames += (uint32_t)frame_count;
      return frame_count;
    }
    if (got > 0) {
      const int32_t partial = got / (int32_t)sizeof(Frame);
      const int32_t remain = frame_count - partial;
      if (remain > 0) {
        memset(frame + partial, 0, (size_t)remain * sizeof(Frame));
      }
      gI2sOkFrames += (uint32_t)partial;
      return frame_count;
    }
  }
#endif

  memset(frame, 0, (size_t)frame_count * sizeof(Frame));
  return frame_count;
}

static bool ssidFilter(const char *ssid, esp_bd_addr_t /*address*/, int rssi) {
  if (ssid == nullptr || ssid[0] == '\0') {
    return false;
  }
  Serial.printf("A2DP: discovered sink '%s' rssi=%d\n", ssid, rssi);

  const char *want = BT_SINK_NAME;
  if (want != nullptr && want[0] != '\0') {
    const bool match = (strcmp(ssid, want) == 0);
    if (match) {
      strncpy(gLastSink, ssid, sizeof(gLastSink) - 1);
    }
    return match;
  }

  strncpy(gLastSink, ssid, sizeof(gLastSink) - 1);
  return true;
}

static void onConnectionState(esp_a2d_connection_state_t state, void *) {
  gBtConnected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);
  const char *name = "unknown";
  switch (state) {
    case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
      name = "DISCONNECTED";
      break;
    case ESP_A2D_CONNECTION_STATE_CONNECTING:
      name = "CONNECTING";
      break;
    case ESP_A2D_CONNECTION_STATE_CONNECTED:
      name = "CONNECTED";
      break;
    case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
      name = "DISCONNECTING";
      break;
  }
  Serial.printf("A2DP: connection %s (sink='%s')\n", name,
                gLastSink[0] ? gLastSink : "(pending)");
}

static void printHelp() {
  Serial.println("Commands: status | vol <0-127> | tone | i2s | probe | help");
}

static void probeI2sPins() {
  int pB = digitalRead(I2S_PIN_BCLK);
  int pL = digitalRead(I2S_PIN_LRCK);
  int pD = digitalRead(I2S_PIN_DIN);
  uint32_t eB = 0, eL = 0, eD = 0;
  const uint32_t t0 = millis();
  while ((millis() - t0) < 2000UL) {
    const int nB = digitalRead(I2S_PIN_BCLK);
    const int nL = digitalRead(I2S_PIN_LRCK);
    const int nD = digitalRead(I2S_PIN_DIN);
    if (nB != pB) {
      eB++;
      pB = nB;
    }
    if (nL != pL) {
      eL++;
      pL = nL;
    }
    if (nD != pD) {
      eD++;
      pD = nD;
    }
  }
  Serial.printf("probe 2s edges: BCLK=%lu LRCK=%lu DIN=%lu levels %d/%d/%d (S3 master drives clocks)\n",
                (unsigned long)eB, (unsigned long)eL, (unsigned long)eD, pB, pL, pD);
}

static void printStatus() {
  Serial.println("---- A2DP Source status ----");
  Serial.printf("  connected: %s\n", gBtConnected ? "yes" : "no");
  Serial.printf("  sink name filter: '%s' (%s)\n", BT_SINK_NAME,
                (BT_SINK_NAME[0] ? "exact match" : "first discovered"));
  Serial.printf("  last sink: '%s'\n", gLastSink[0] ? gLastSink : "(none)");
  Serial.printf("  volume: %u / 127\n", (unsigned)gVolume);
  Serial.printf("  audio mode: %s\n",
                gForceTone ? "force tone" : "I2S prefer + silence when idle");
  Serial.printf("  tone frames: %lu  i2s frames: %lu\n", (unsigned long)gToneFrames,
                (unsigned long)gI2sOkFrames);
  Serial.printf("  I2S ready: %s  SLAVE RX pins BCLK=%d LRCK=%d DIN=%d\n",
                gI2sReady ? "yes" : "no", I2S_PIN_BCLK, I2S_PIN_LRCK, I2S_PIN_DIN);
  Serial.printf("  S3 expect master TX BCLK=%d LRCK=%d DOUT=%d (GND common)\n",
                S3_I2S_PIN_BCLK_SUGGEST, S3_I2S_PIN_LRCK_SUGGEST, S3_I2S_PIN_DOUT_SUGGEST);
  Serial.println("----------------------------");
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
  if (strcmp(line, "tone") == 0) {
    gForceTone = true;
    Serial.println("Audio: force test tone");
    return;
  }
  if (strcmp(line, "i2s") == 0) {
    gForceTone = false;
    Serial.println("Audio: prefer I2S, silence if idle");
    return;
  }
  if (strcmp(line, "probe") == 0) {
    probeI2sPins();
    return;
  }
  if (strncmp(line, "vol ", 4) == 0) {
    const int v = atoi(line + 4);
    if (v < 0 || v > 127) {
      Serial.println("vol: use 0..127");
      return;
    }
    gVolume = (uint8_t)v;
    gA2dp.set_volume(gVolume);
    Serial.printf("volume set to %u\n", (unsigned)gVolume);
    return;
  }
  Serial.printf("Unknown: '%s' (try help)\n", line);
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println();
  Serial.println("ESP32-WROOM A2DP Source bridge");
  Serial.println("S3 radio (I2S master TX) -> this board (I2S slave RX) -> Classic BT");
  printHelp();

  {
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 30000,
        .idle_core_mask = 0,
        .trigger_panic = false,
    };
    esp_err_t twdt_err = esp_task_wdt_reconfigure(&twdt_config);
    Serial.printf("TWDT reconfigure: %s\n", esp_err_to_name(twdt_err));
  }

#if A2DP_USE_I2S_INPUT
  gI2sReady = i2sInitSlaveRx();
#else
  gI2sReady = false;
  Serial.println("I2S: disabled (tone only build)");
#endif

  gA2dp.set_ssid_callback(ssidFilter);
  gA2dp.set_on_connection_state_changed(onConnectionState);
  gA2dp.set_data_callback_in_frames(getDataFrames);
  gA2dp.set_volume(gVolume);
  gA2dp.set_ssp_enabled(true);
  gA2dp.set_auto_reconnect(false);
  gA2dp.set_pin_code("0000", ESP_BT_PIN_TYPE_FIXED);

  if (BT_SINK_NAME[0] != '\0') {
    Serial.printf("A2DP: starting, target sink '%s'\n", BT_SINK_NAME);
    gA2dp.start(BT_SINK_NAME);
  } else {
    Serial.println("A2DP: starting, auto-connect first discovered sink");
    gA2dp.start();
  }

  printStatus();
}

void loop() {
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

  const uint32_t now = millis();
  if (now - lastBeat >= 5000) {
    lastBeat = now;
#if A2DP_USE_I2S_INPUT
    size_t diagGot = 0;
    if (gI2sReady) {
      uint8_t diagBuf[256];
      (void)i2s_read(kI2sPort, diagBuf, sizeof(diagBuf), &diagGot, pdMS_TO_TICKS(20));
    }
    Serial.printf("beat: bt=%s sink='%s' vol=%u toneF=%lu i2sF=%lu forceTone=%d diagGot=%u\n",
                  gBtConnected ? "up" : "down", gLastSink[0] ? gLastSink : "-",
                  (unsigned)gVolume, (unsigned long)gToneFrames, (unsigned long)gI2sOkFrames,
                  (int)gForceTone, (unsigned)diagGot);
#else
    Serial.printf("beat: bt=%s sink='%s' vol=%u toneF=%lu i2sF=%lu forceTone=%d\n",
                  gBtConnected ? "up" : "down", gLastSink[0] ? gLastSink : "-",
                  (unsigned)gVolume, (unsigned long)gToneFrames, (unsigned long)gI2sOkFrames,
                  (int)gForceTone);
#endif
  }

  delay(10);
}
