#ifndef PSP_AUDIO_HLE_H
#define PSP_AUDIO_HLE_H

#include <stdbool.h>
#include <stdint.h>
#include "../memory/psp_memory.h"
#include "../cpu/psp_cpu.h"

#define PSP_AUDIO_CHANNEL_COUNT 8u
#define PSP_AUDIO_MIN_SAMPLES 64u
#define PSP_AUDIO_MAX_SAMPLES 2048u
#define PSP_AUDIO_MIX_BUFFER_FRAMES 8192u

typedef struct {
    bool reserved;
    uint32_t sample_count;
    uint32_t format;
    uint32_t left_volume;
    uint32_t right_volume;
    int16_t samples[PSP_AUDIO_MIX_BUFFER_FRAMES * 2u];
    uint32_t read_frame;
    uint32_t write_frame;
    uint32_t queued_frames;
} PSP_Audio_Channel;

typedef struct {
    PSP_Audio_Channel channels[PSP_AUDIO_CHANNEL_COUNT];
    uint32_t dropped_frames;
    uint32_t mixed_frames;
} PSP_Audio_HLE;

void psp_audio_hle_init(PSP_Audio_HLE *audio);
void psp_audio_hle_reset(PSP_Audio_HLE *audio);
int32_t psp_audio_hle_reserve(PSP_Audio_HLE *audio, int32_t channel,
                              uint32_t sample_count, uint32_t frequency);
int32_t psp_audio_hle_release(PSP_Audio_HLE *audio, int32_t channel);
int32_t psp_audio_hle_set_length(PSP_Audio_HLE *audio, int32_t channel, uint32_t sample_count);
int32_t psp_audio_hle_set_format(PSP_Audio_HLE *audio, int32_t channel, uint32_t format);
int32_t psp_audio_hle_set_volume(PSP_Audio_HLE *audio, int32_t channel, uint32_t left, uint32_t right);
int32_t psp_audio_hle_get_rest(PSP_Audio_HLE *audio, int32_t channel);
int32_t psp_audio_hle_output(PSP_Audio_HLE *audio, PSP_Memory *memory,
                              int32_t channel, uint32_t left_volume,
                              uint32_t right_volume, uint32_t address);
uint32_t psp_audio_hle_read(PSP_Audio_HLE *audio, int16_t *stereo,
                            uint32_t max_frames);
bool psp_audio_hle_register(PSP_Audio_HLE *audio);
void psp_audio_hle_unregister(void);

#define PSP_NID_AUDIO_CH_RESERVE 0x5EC81C55u
#define PSP_NID_AUDIO_CH_RELEASE 0x6FC46853u
#define PSP_NID_AUDIO_OUTPUT_BLOCKING 0x136CAF51u
#define PSP_NID_AUDIO_OUTPUT_PANNED_BLOCKING 0x13F592BCu
#define PSP_NID_AUDIO_GET_REST_LENGTH 0xB011922Fu
#define PSP_NID_AUDIO_SET_DATA_LENGTH 0xCB2E439Eu
#define PSP_NID_AUDIO_CHANGE_CONFIG 0x95FD0C2Du
#define PSP_NID_AUDIO_CHANGE_VOLUME 0xB7E1D8E7u
#define PSP_SYSCALL_AUDIO_CH_RESERVE 0x120u
#define PSP_SYSCALL_AUDIO_CH_RELEASE 0x121u
#define PSP_SYSCALL_AUDIO_OUTPUT_BLOCKING 0x122u
#define PSP_SYSCALL_AUDIO_OUTPUT_PANNED_BLOCKING 0x123u
#define PSP_SYSCALL_AUDIO_GET_REST_LENGTH 0x124u
#define PSP_SYSCALL_AUDIO_SET_DATA_LENGTH 0x125u
#define PSP_SYSCALL_AUDIO_CHANGE_CONFIG 0x126u
#define PSP_SYSCALL_AUDIO_CHANGE_VOLUME 0x127u

#endif
