#include "psp_audio.h"

#include <3ds.h>

static bool audio_ready = false;

bool psp_audio_init(void)
{
    if (audio_ready)
        return true;

    if (R_FAILED(ndspInit()))
        return false;

    ndspSetOutputMode(NDSP_OUTPUT_STEREO);

    ndspChnReset(0);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, 44100.0f);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    ndspChnSetPaused(0, false);

    audio_ready = true;
    return true;
}

void psp_audio_shutdown(void)
{
    if (!audio_ready)
        return;

    ndspChnReset(0);
    ndspExit();

    audio_ready = false;
}

bool psp_audio_is_ready(void)
{
    return audio_ready;
}
