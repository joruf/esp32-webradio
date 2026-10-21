"""
Patch ESP32-audioI2S setPinout() for RADIO_I2S_A2DP_TX decode-only mode.

Enabling a dummy I2S master on the S3 Touch LCD board trips TG1WDT within ~1s.
The webradio bridge owns the wire I2S slave TX; Audio only needs buffers + task.
"""

Import("env")  # noqa: F821  # PlatformIO
from pathlib import Path

MARKER = "RADIO_I2S_A2DP_DECODE_ONLY_PATCH"
LIB = Path(env["PROJECT_LIBDEPS_DIR"]) / env["PIOENV"] / "ESP32-audioI2S" / "src" / "Audio.cpp"

if not LIB.is_file():
    print(f"patch_audio_i2s: skip, missing {LIB}")
else:
    text = LIB.read_text(encoding="utf-8", errors="replace")
    if MARKER in text:
        print("patch_audio_i2s: already applied")
    else:
        old = """bool Audio::setPinout(uint8_t BCLK, uint8_t LRC, uint8_t DOUT, int8_t MCLK) {

    i2s_std_gpio_config_t gpio_cfg = {};
    bool                  result = i2s_config();
    uint32_t              vu_delay_frames = 0;

    i2s_event_callbacks_t cbs = {};
    cbs.on_sent = i2s_tx_sent_callback;
    ESP_ERROR_CHECK(i2s_channel_register_event_callback(m_i2s_tx_handle, &cbs, this));

    gpio_cfg.bclk = (gpio_num_t)BCLK;
    gpio_cfg.din = (gpio_num_t)I2S_GPIO_UNUSED;
    gpio_cfg.dout = (gpio_num_t)DOUT;
    gpio_cfg.mclk = (gpio_num_t)MCLK;
    gpio_cfg.ws = (gpio_num_t)LRC;

#if (ESP_ARDUINO_VERSION_MAJOR < 3)
    AUDIO_LOG_ERROR("Arduino Version must be 3.0.0 or higher!");
    result = false;
    goto exit;
#endif

    m_f_psramFound = psramInit();
    if (!m_f_psramFound) {
        AUDIO_LOG_ERROR("PSRAM not found");
        result = false;
        goto exit;
    }
    m_work_words = settings.DMA_FRAME_NUM * 2;
    SamplesBuff.setBufsize(128 * settings.DMA_FRAME_NUM); // must be a multiple of DMA_FRAME_NUM
    if (SamplesBuff.init() == 0) {
        AUDIO_LOG_ERROR("RingBuffer allocation failed");
        result = false;
        goto exit;
    }

    m_outBuff.alloc_array(m_outbuffSize, "m_outBuff");
    m_resamplesBuff.alloc_array(m_resamplesBuffSize, "m_resamplesBuff");
    m_i2sWorkBuff.alloc_array(m_work_words, "i2sWorkBuff");
    m_metadataBuff.alloc(4096 + 1, "m_metadataBuff");   // max 4096 + 1 for null terminator, just to make library code 'safe'
    m_httpRespHdrBuff.alloc(4096, "m_httpRespHdrBuff"); // enough space to store http response header

    if (!m_outBuff.valid() || !m_resamplesBuff.valid() || !m_i2sWorkBuff.valid() || !m_metadataBuff.valid() || !m_httpRespHdrBuff.valid()) {
        result = false;
        goto exit;
    }

    I2Sstop();
    if (i2s_channel_reconfig_std_gpio(m_i2s_tx_handle, &gpio_cfg) != ESP_OK) {
        result = false;
        goto exit;
    }
    I2Sstart();

exit:

    calculateVolumeLimits(); // first init, vol = 21, vol_steps = 21
    startAudioTask();

    m_f_I2S_init = result;
    return m_f_I2S_init;
}"""

        new = """bool Audio::setPinout(uint8_t BCLK, uint8_t LRC, uint8_t DOUT, int8_t MCLK) {
#if defined(RADIO_I2S_A2DP_TX) /* RADIO_I2S_A2DP_DECODE_ONLY_PATCH */
    /* Decode-only: allocate PSRAM buffers + audio task, no I2S peripheral.
     * Wire PCM is owned by the webradio I2S slave TX + audio_process_i2s tap. */
    (void)BCLK;
    (void)LRC;
    (void)DOUT;
    (void)MCLK;
    bool result = true;
#if (ESP_ARDUINO_VERSION_MAJOR < 3)
    AUDIO_LOG_ERROR("Arduino Version must be 3.0.0 or higher!");
    result = false;
    goto exit_decode_only;
#endif
    m_f_psramFound = psramInit();
    if (!m_f_psramFound) {
        AUDIO_LOG_ERROR("PSRAM not found");
        result = false;
        goto exit_decode_only;
    }
    m_work_words = settings.DMA_FRAME_NUM * 2;
    SamplesBuff.setBufsize(128 * settings.DMA_FRAME_NUM);
    if (SamplesBuff.init() == 0) {
        AUDIO_LOG_ERROR("RingBuffer allocation failed");
        result = false;
        goto exit_decode_only;
    }
    m_outBuff.alloc_array(m_outbuffSize, "m_outBuff");
    m_resamplesBuff.alloc_array(m_resamplesBuffSize, "m_resamplesBuff");
    m_i2sWorkBuff.alloc_array(m_work_words, "i2sWorkBuff");
    m_metadataBuff.alloc(4096 + 1, "m_metadataBuff");
    m_httpRespHdrBuff.alloc(4096, "m_httpRespHdrBuff");
    if (!m_outBuff.valid() || !m_resamplesBuff.valid() || !m_i2sWorkBuff.valid() || !m_metadataBuff.valid() ||
        !m_httpRespHdrBuff.valid()) {
        result = false;
        goto exit_decode_only;
    }
    m_dmaFreeDesc = settings.DMA_DESC_NUM;
exit_decode_only:
    calculateVolumeLimits();
    startAudioTask();
    m_f_I2S_init = result;
    return m_f_I2S_init;
#else
    i2s_std_gpio_config_t gpio_cfg = {};
    bool                  result = i2s_config();
    uint32_t              vu_delay_frames = 0;

    i2s_event_callbacks_t cbs = {};
    cbs.on_sent = i2s_tx_sent_callback;
    ESP_ERROR_CHECK(i2s_channel_register_event_callback(m_i2s_tx_handle, &cbs, this));

    gpio_cfg.bclk = (gpio_num_t)BCLK;
    gpio_cfg.din = (gpio_num_t)I2S_GPIO_UNUSED;
    gpio_cfg.dout = (gpio_num_t)DOUT;
    gpio_cfg.mclk = (gpio_num_t)MCLK;
    gpio_cfg.ws = (gpio_num_t)LRC;

#if (ESP_ARDUINO_VERSION_MAJOR < 3)
    AUDIO_LOG_ERROR("Arduino Version must be 3.0.0 or higher!");
    result = false;
    goto exit;
#endif

    m_f_psramFound = psramInit();
    if (!m_f_psramFound) {
        AUDIO_LOG_ERROR("PSRAM not found");
        result = false;
        goto exit;
    }
    m_work_words = settings.DMA_FRAME_NUM * 2;
    SamplesBuff.setBufsize(128 * settings.DMA_FRAME_NUM); // must be a multiple of DMA_FRAME_NUM
    if (SamplesBuff.init() == 0) {
        AUDIO_LOG_ERROR("RingBuffer allocation failed");
        result = false;
        goto exit;
    }

    m_outBuff.alloc_array(m_outbuffSize, "m_outBuff");
    m_resamplesBuff.alloc_array(m_resamplesBuffSize, "m_resamplesBuff");
    m_i2sWorkBuff.alloc_array(m_work_words, "i2sWorkBuff");
    m_metadataBuff.alloc(4096 + 1, "m_metadataBuff");   // max 4096 + 1 for null terminator, just to make library code 'safe'
    m_httpRespHdrBuff.alloc(4096, "m_httpRespHdrBuff"); // enough space to store http response header

    if (!m_outBuff.valid() || !m_resamplesBuff.valid() || !m_i2sWorkBuff.valid() || !m_metadataBuff.valid() || !m_httpRespHdrBuff.valid()) {
        result = false;
        goto exit;
    }

    I2Sstop();
    if (i2s_channel_reconfig_std_gpio(m_i2s_tx_handle, &gpio_cfg) != ESP_OK) {
        result = false;
        goto exit;
    }
    I2Sstart();

exit:

    calculateVolumeLimits(); // first init, vol = 21, vol_steps = 21
    startAudioTask();

    m_f_I2S_init = result;
    return m_f_I2S_init;
#endif /* RADIO_I2S_A2DP_TX */
}"""

        if old not in text:
            print("patch_audio_i2s: ERROR - setPinout() pattern not found")
            env.Exit(1)  # noqa: F821
        else:
            LIB.write_text(text.replace(old, new, 1), encoding="utf-8")
            print(f"patch_audio_i2s: patched {LIB}")
