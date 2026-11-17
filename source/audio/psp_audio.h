#ifndef PSP_AUDIO_H
#define PSP_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

bool psp_audio_init(void);
void psp_audio_shutdown(void);
bool psp_audio_is_ready(void);

bool psp_audio_submit(
    const int16_t *stereo_samples,
    unsigned int frames
);

#endif
