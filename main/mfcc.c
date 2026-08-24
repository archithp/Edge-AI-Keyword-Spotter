#include "mfcc.h"
#include <math.h>
#include <string.h>
#include <assert.h>
#include "kiss_fft.h"
#include "esp_log.h"

static const char *TAG = "DSP";

float mfcc_out[13][49];


/* --- static tables (computed once at init) --- */
static float  window[FRAME_SIZE];
static float  mel_filter[N_MELS][FFT_SIZE / 2 + 1];
static double dct[N_MFCC][N_MELS];
static kiss_fft_cfg fft_cfg;

/* --- static FFT buffers (keeps stack usage low on ESP32) --- */
static kiss_fft_cpx fft_in[FFT_SIZE];
static kiss_fft_cpx fft_out[FFT_SIZE];

static float hz_to_mel(float hz)
{
    return 2595.0f * log10f(1.0f + hz / 700.0f);
}

static float mel_to_hz(float mel)
{
    return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f);
}

static void create_window(void)
{
    for (int i = 0; i < FRAME_SIZE; i++) {
        window[i] = 0.5f - 0.5f * cosf((2.0f * PI * i) / (FRAME_SIZE - 1));
    }
}

static void create_mel_filter(void)
{
    float mel_min = hz_to_mel(LOW_FREQ);
    float mel_max = hz_to_mel(HIGH_FREQ);
    float mel_points[N_MELS + 2];
    int   bins[N_MELS + 2];

    for (int i = 0; i < N_MELS + 2; i++) {
        mel_points[i] = mel_min + i * (mel_max - mel_min) / (N_MELS + 1);
        float hz = mel_to_hz(mel_points[i]);
        bins[i] = (int)floorf((FFT_SIZE + 1) * hz / SAMPLE_RATE);
    }

    memset(mel_filter, 0, sizeof(mel_filter));

    for (int m = 1; m <= N_MELS; m++) {
        int left   = bins[m - 1];
        int center = bins[m];
        int right  = bins[m + 1];

        if (center > left) {
            for (int k = left; k < center; k++) {
                mel_filter[m - 1][k] = (float)(k - left) / (float)(center - left);
            }
        }
        if (right > center) {
            for (int k = center; k < right; k++) {
                mel_filter[m - 1][k] = (float)(right - k) / (float)(right - center);
            }
        }
    }
}

static void create_dct(void)
{
    for (int k = 0; k < N_MFCC; k++) {
        for (int n = 0; n < N_MELS; n++) {
            dct[k][n] = cos(PI * k * (2 * n + 1) / (2.0 * N_MELS));
        }
    }
}

/* ------------------------------------------------------------------ */
void mfcc_init(void)
{
    create_window();
    create_mel_filter();
    create_dct();

    fft_cfg = kiss_fft_alloc(FFT_SIZE, 0, NULL, NULL);
    assert(fft_cfg != NULL);
}

/* ------------------------------------------------------------------ */
void pcm_to_mfcc(const int16_t *pcm)
{
    ESP_LOGI(TAG, "MFCC computation started");

    int frames = 1 + (MAX_PCM_SAMPLES - FRAME_SIZE) / FRAME_STEP;
    assert(frames <= MAX_FRAMES);

    for (int f = 0; f < frames; f++) {
        /* 1. zero input */
        memset(fft_in, 0, sizeof(fft_in));

        /* 2. window + pre-emphasis + int16->float normalization on-the-fly */
        for (int i = 0; i < FRAME_SIZE; i++) {
            int idx = f * FRAME_STEP + i;

            float current = pcm[idx] / 32768.0f;

            float sample = current;
            if (idx > 0) {
                float prev = pcm[idx - 1] / 32768.0f;
                sample -= PRE_EMPHASIS * prev;
            }

            fft_in[i].r = sample * window[i]; 
            fft_in[i].i = 0.0f;
        }

        /* 3. FFT */
        kiss_fft(fft_cfg, fft_in, fft_out);

        /* 4. power spectrum */
        float power[FFT_SIZE / 2 + 1];
        for (int i = 0; i <= FFT_SIZE / 2; i++) {
            power[i] = (fft_out[i].r * fft_out[i].r +
                        fft_out[i].i * fft_out[i].i) / FFT_SIZE;
        }

        /* 5. mel filterbank + log */
        double logmel[N_MELS];
        for (int m = 0; m < N_MELS; m++) {
            float sum = 0.0f;
            for (int k = 0; k <= FFT_SIZE / 2; k++) {
                sum += mel_filter[m][k] * power[k];
            }
            if (sum < 1e-10f) sum = 1e-10f;
            logmel[m] = log((double)sum);
        }

        /* 6. DCT  -> output [coeff][frame] */
        for (int k = 0; k < N_MFCC; k++) {
            double value = 0.0;
            for (int n = 0; n < N_MELS; n++) {
                value += dct[k][n] * logmel[n];
            }
            mfcc_out[k][f] = (float)value;
        }
    }

    ESP_LOGI(TAG, "MFCC computation complete");
}