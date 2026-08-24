#ifndef MFCC_H
#define MFCC_H

#include <stdint.h>

/* Audio & FFT parameters */
#define SAMPLE_RATE     16000
#define FRAME_SIZE      400
#define FRAME_STEP      320
#define FFT_SIZE        512
#define N_MELS          26
#define N_MFCC          13
#define LOW_FREQ        20
#define HIGH_FREQ       4000
#define PRE_EMPHASIS    0.97f
#define PI              3.14159265358979323846f

#define MAX_PCM_SAMPLES 16000
#define MAX_FRAMES      49

void mfcc_init(void);
void pcm_to_mfcc(const int16_t *pcm);

#endif