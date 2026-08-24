#include "i2s_mic.h"

#include "driver/i2s_std.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SAMPLE_RATE     16000

#define I2S_BCLK        GPIO_NUM_16
#define I2S_WS          GPIO_NUM_17
#define I2S_DIN         GPIO_NUM_18


int16_t pcm_buffer[AUDIO_SAMPLES];

static const char *TAG = "I2S_MIC";

static i2s_chan_handle_t rx_handle = NULL;

void i2s_mic_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_32BIT,
                        I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK,
            .ws   = I2S_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = I2S_DIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    /* INMP441 L/R = GND → LEFT SLOT */
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

    ESP_LOGI(TAG, "INMP441 initialized");
}

size_t i2s_mic_read(int16_t *pcm, size_t samples)
{
    static int32_t raw[512];
    size_t bytes_read = 0;

    ESP_ERROR_CHECK(
        i2s_channel_read(
            rx_handle,
            raw,
            samples * sizeof(int32_t),
            &bytes_read,
            portMAX_DELAY));

    size_t count = bytes_read / sizeof(int32_t);

    for (size_t i = 0; i < count; i++){
        pcm[i] = (int16_t)(raw[i] >> 14);
    }

    return count;
}

void audio_capture(void)
{
    ESP_LOGI(TAG, "Audio capture started");
    size_t received = 0;

    while (received < AUDIO_SAMPLES)
    {
        size_t remaining = AUDIO_SAMPLES - received;
        size_t to_read = (remaining > 512) ? 512 : remaining;

        received += i2s_mic_read(&pcm_buffer[received], to_read);
    }
    ESP_LOGI(TAG, "Audio capture complete");
}