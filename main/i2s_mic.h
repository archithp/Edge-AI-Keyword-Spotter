#ifndef I2S_MIC_H
#define I2S_MIC_H

#include <stdint.h>
#include <stddef.h>

#define AUDIO_SAMPLES   16000

void i2s_mic_init(void);
size_t i2s_mic_read(int16_t *pcm, size_t samples);
void audio_capture(void);

#endif