#ifndef PSP_AUDIO_HLE_H
#define PSP_AUDIO_HLE_H

#include <stdbool.h>
#include <stdint.h>
#include "../memory/psp_memory.h"
#include "../cpu/psp_cpu.h"

#define PSP_AUDIO_CHANNEL_COUNT 8u
#define PSP_AUDIO_MAX_SAMPLES 2048u
#define PSP_AUDIO_MIX_BUFFER_FRAMES 8192u

typedef struct {
    bool reserved;
    uint32_t sample_count;
    uint32_t frequency;
    uint32_t left_volume;
    uint32_t right_volume;
} PSP_Audio_Channel;

typedef struct {
    PSP_Audio_Channel channels[PSP_AUDIO_CHANNEL_COUNT];
    int16_t mix_buffer[PSP_AUDIO_MIX_BUFFER_FRAMES * 2u];
    uint32_t read_frame;
    uint32_t write_frame;
    uint32_t queued_frames;
    uint32_t dropped_frames;
    uint32_t mixed_frames;
} PSP_Audio_HLE;

void psp_audio_hle_init(PSP_Audio_HLE *audio);
void psp_audio_hle_reset(PSP_Audio_HLE *audio);
int32_t psp_audio_hle_reserve(PSP_Audio_HLE *audio, int32_t channel,
                              uint32_t sample_count, uint32_t frequency);
int32_t psp_audio_hle_release(PSP_Audio_HLE *audio, int32_t channel);
int32_t psp_audio_hle_output(PSP_Audio_HLE *audio, PSP_Memory *memory,
                              int32_t channel, uint32_t left_volume,
                              uint32_t right_volume, uint32_t address);
uint32_t psp_audio_hle_read(PSP_Audio_HLE *audio, int16_t *stereo,
                            uint32_t max_frames);
bool psp_audio_hle_register(PSP_Audio_HLE *audio);
void psp_audio_hle_unregister(void);

#define PSP_NID_AUDIO_CH_RESERVE 0x5EC81C55u
#define PSP_NID_AUDIO_CH_RELEASE 0x6FC46853u
#define PSP_NID_AUDIO_OUTPUT_BLOCKING 0x8C1009B2u
#define PSP_NID_AUDIO_OUTPUT_PANNED_BLOCKING 0x13F592BCu
#define PSP_SYSCALL_AUDIO_CH_RESERVE 0x120u
#define PSP_SYSCALL_AUDIO_CH_RELEASE 0x121u
#define PSP_SYSCALL_AUDIO_OUTPUT_BLOCKING 0x122u
#define PSP_SYSCALL_AUDIO_OUTPUT_PANNED_BLOCKING 0x123u

#endif
