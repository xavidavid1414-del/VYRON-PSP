#include "psp_audio.h"

#include <3ds.h>
#include <string.h>

#define AUDIO_CHANNEL 0
#define AUDIO_RATE 44100
#define AUDIO_BUFFER_COUNT 3
#define AUDIO_MAX_FRAMES 1024
#define AUDIO_CHANNELS 2

#define AUDIO_BUFFER_BYTES \
    (AUDIO_MAX_FRAMES * AUDIO_CHANNELS * sizeof(s16))

static bool audio_ready = false;

static s16 *audio_buffers[AUDIO_BUFFER_COUNT];
static ndspWaveBuf wave_buffers[AUDIO_BUFFER_COUNT];

bool psp_audio_init(void)
{
    if (audio_ready)
        return true;

    if (R_FAILED(ndspInit()))
        return false;

    for (int i = 0; i < AUDIO_BUFFER_COUNT; i++) {
        audio_buffers[i] = linearAlloc(AUDIO_BUFFER_BYTES);

        if (!audio_buffers[i]) {
            for (int j = 0; j < i; j++) {
                linearFree(audio_buffers[j]);
                audio_buffers[j] = NULL;
            }

            ndspExit();
            return false;
        }

        memset(&wave_buffers[i], 0, sizeof(ndspWaveBuf));
    }

    ndspSetOutputMode(NDSP_OUTPUT_STEREO);

    ndspChnReset(AUDIO_CHANNEL);
    ndspChnSetInterp(AUDIO_CHANNEL, NDSP_INTERP_LINEAR);
    ndspChnSetRate(AUDIO_CHANNEL, (float)AUDIO_RATE);
    ndspChnSetFormat(
        AUDIO_CHANNEL,
        NDSP_FORMAT_STEREO_PCM16
    );

    ndspChnSetPaused(AUDIO_CHANNEL, false);

    audio_ready = true;
    return true;
}

bool psp_audio_submit(
    const int16_t *stereo_samples,
    unsigned int frames
)
{
    if (!audio_ready ||
        !stereo_samples ||
        frames == 0 ||
        frames > AUDIO_MAX_FRAMES)
        return false;

    for (int i = 0; i < AUDIO_BUFFER_COUNT; i++) {
        ndspWaveBuf *wave = &wave_buffers[i];

        if (wave->status != NDSP_WBUF_FREE &&
            wave->status != NDSP_WBUF_DONE)
            continue;

        unsigned int sample_count =
            frames * AUDIO_CHANNELS;

        unsigned int byte_count =
            sample_count * sizeof(s16);

        memcpy(
            audio_buffers[i],
            stereo_samples,
            byte_count
        );

        DSP_FlushDataCache(
            audio_buffers[i],
            byte_count
        );

        memset(wave, 0, sizeof(ndspWaveBuf));

        wave->data_pcm16 = audio_buffers[i];
        wave->nsamples = sample_count;
        wave->looping = false;

        ndspChnWaveBufAdd(AUDIO_CHANNEL, wave);

        return true;
    }

    return false;
}

void psp_audio_shutdown(void)
{
    if (!audio_ready)
        return;

    ndspChnWaveBufClear(AUDIO_CHANNEL);
    ndspChnReset(AUDIO_CHANNEL);

    for (int i = 0; i < AUDIO_BUFFER_COUNT; i++) {
        if (audio_buffers[i]) {
            linearFree(audio_buffers[i]);
            audio_buffers[i] = NULL;
        }

        memset(&wave_buffers[i], 0, sizeof(ndspWaveBuf));
    }

    ndspExit();
    audio_ready = false;
}

bool psp_audio_is_ready(void)
{
    return audio_ready;
}
