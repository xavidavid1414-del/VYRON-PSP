#ifndef PSP_AUDIO_H
#define PSP_AUDIO_H

#include <stdbool.h>

bool psp_audio_init(void);
void psp_audio_shutdown(void);
bool psp_audio_is_ready(void);

#endif
