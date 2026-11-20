#include "psp_audio_hle.h"
#include "../cpu/psp_syscall.h"
#include "../loader/psp_nid.h"
#include <string.h>

#define PSP_AUDIO_MAX_VOLUME 0x8000u

static PSP_Audio_HLE *active_audio;

static int16_t clamp_sample(int32_t value)
{
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (int16_t)value;
}

void psp_audio_hle_init(PSP_Audio_HLE *audio)
{
    if (audio) memset(audio, 0, sizeof(*audio));
}

void psp_audio_hle_reset(PSP_Audio_HLE *audio)
{
    if (!audio) return;
    memset(audio, 0, sizeof(*audio));
}

int32_t psp_audio_hle_reserve(PSP_Audio_HLE *audio, int32_t channel,
                               uint32_t sample_count, uint32_t format)
{
    if (!audio || sample_count < PSP_AUDIO_MIN_SAMPLES ||
        sample_count > PSP_AUDIO_MAX_SAMPLES || (sample_count & 63u) != 0u ||
        (format != 0u && format != 0x10u))
        return -1;

    if (channel == -1) {
        for (uint32_t i = 0; i < PSP_AUDIO_CHANNEL_COUNT; ++i) {
            if (!audio->channels[i].reserved) {
                channel = (int32_t)i;
                break;
            }
        }
    }

    if (channel < 0 || (uint32_t)channel >= PSP_AUDIO_CHANNEL_COUNT)
        return -1;

    PSP_Audio_Channel *entry = &audio->channels[channel];
    if (entry->reserved) return -1;

    entry->reserved = true;
    entry->sample_count = sample_count;
    entry->format = format;
    entry->left_volume = PSP_AUDIO_MAX_VOLUME;
    entry->right_volume = PSP_AUDIO_MAX_VOLUME;
    return channel;
}

int32_t psp_audio_hle_release(PSP_Audio_HLE *audio, int32_t channel)
{
    if (!audio || channel < 0 ||
        (uint32_t)channel >= PSP_AUDIO_CHANNEL_COUNT ||
        !audio->channels[channel].reserved)
        return -1;
    memset(&audio->channels[channel], 0, sizeof(audio->channels[channel]));
    return 0;
}

int32_t psp_audio_hle_set_length(PSP_Audio_HLE *audio, int32_t channel,
                                  uint32_t sample_count)
{
    if (!audio || channel < 0 || (uint32_t)channel >= PSP_AUDIO_CHANNEL_COUNT ||
        !audio->channels[channel].reserved ||
        sample_count < PSP_AUDIO_MIN_SAMPLES ||
        sample_count > PSP_AUDIO_MAX_SAMPLES || (sample_count & 63u) != 0u)
        return -1;
    audio->channels[channel].sample_count = sample_count;
    return 0;
}

int32_t psp_audio_hle_set_format(PSP_Audio_HLE *audio, int32_t channel,
                                  uint32_t format)
{
    if (!audio || channel < 0 || (uint32_t)channel >= PSP_AUDIO_CHANNEL_COUNT ||
        !audio->channels[channel].reserved ||
        (format != 0u && format != 0x10u))
        return -1;
    audio->channels[channel].format = format;
    return 0;
}

int32_t psp_audio_hle_set_volume(PSP_Audio_HLE *audio, int32_t channel,
                                  uint32_t left, uint32_t right)
{
    if (!audio || channel < 0 || (uint32_t)channel >= PSP_AUDIO_CHANNEL_COUNT ||
        !audio->channels[channel].reserved ||
        left > PSP_AUDIO_MAX_VOLUME || right > PSP_AUDIO_MAX_VOLUME)
        return -1;
    audio->channels[channel].left_volume = left;
    audio->channels[channel].right_volume = right;
    return 0;
}

int32_t psp_audio_hle_get_rest(PSP_Audio_HLE *audio, int32_t channel)
{
    if (!audio || channel < 0 || (uint32_t)channel >= PSP_AUDIO_CHANNEL_COUNT ||
        !audio->channels[channel].reserved)
        return -1;
    return (int32_t)audio->channels[channel].queued_frames;
}

int32_t psp_audio_hle_output(PSP_Audio_HLE *audio, PSP_Memory *memory,
                              int32_t channel, uint32_t left_volume,
                              uint32_t right_volume, uint32_t address)
{
    if (!audio || !memory || channel < 0 ||
        (uint32_t)channel >= PSP_AUDIO_CHANNEL_COUNT ||
        !audio->channels[channel].reserved)
        return -1;

    PSP_Audio_Channel *entry = &audio->channels[channel];
    uint32_t frames = entry->sample_count;
    uint32_t bytes_per_frame = entry->format == 0u ? 4u : 2u;
    if (frames == 0 || frames > PSP_AUDIO_MAX_SAMPLES ||
        !psp_memory_is_valid(memory, address, frames * bytes_per_frame))
        return -1;

    if (left_volume > PSP_AUDIO_MAX_VOLUME) left_volume = PSP_AUDIO_MAX_VOLUME;
    if (right_volume > PSP_AUDIO_MAX_VOLUME) right_volume = PSP_AUDIO_MAX_VOLUME;
    left_volume = (left_volume * entry->left_volume) / PSP_AUDIO_MAX_VOLUME;
    right_volume = (right_volume * entry->right_volume) / PSP_AUDIO_MAX_VOLUME;

    for (uint32_t i = 0; i < frames; ++i) {
        uint32_t offset = address + i * bytes_per_frame;
        int16_t sample_left = (int16_t)((uint16_t)psp_memory_read8(memory, offset) |
                              ((uint16_t)psp_memory_read8(memory, offset + 1u) << 8));
        int16_t sample_right = sample_left;
        if (entry->format == 0u) {
            uint32_t right_offset = offset + 2u;
            sample_right = (int16_t)((uint16_t)psp_memory_read8(memory, right_offset) |
                           ((uint16_t)psp_memory_read8(memory, right_offset + 1u) << 8));
        }
        if (entry->queued_frames == PSP_AUDIO_MIX_BUFFER_FRAMES) {
            entry->read_frame = (entry->read_frame + 1u) % PSP_AUDIO_MIX_BUFFER_FRAMES;
            entry->queued_frames--;
            audio->dropped_frames++;
        }
        uint32_t slot = entry->write_frame;
        entry->samples[slot * 2u] = clamp_sample(
            ((int32_t)sample_left * (int32_t)left_volume) / (int32_t)PSP_AUDIO_MAX_VOLUME);
        entry->samples[slot * 2u + 1u] = clamp_sample(
            ((int32_t)sample_right * (int32_t)right_volume) / (int32_t)PSP_AUDIO_MAX_VOLUME);
        entry->write_frame = (entry->write_frame + 1u) % PSP_AUDIO_MIX_BUFFER_FRAMES;
        entry->queued_frames++;
        audio->mixed_frames++;
    }

    return 0;
}

uint32_t psp_audio_hle_read(PSP_Audio_HLE *audio, int16_t *stereo,
                             uint32_t max_frames)
{
    if (!audio || !stereo || max_frames == 0) return 0;

    for (uint32_t frame = 0; frame < max_frames; ++frame) {
        int32_t left = 0;
        int32_t right = 0;
        bool any = false;

        for (uint32_t ch = 0; ch < PSP_AUDIO_CHANNEL_COUNT; ++ch) {
            PSP_Audio_Channel *entry = &audio->channels[ch];
            if (!entry->reserved || entry->queued_frames == 0)
                continue;

            uint32_t slot = entry->read_frame;
            left += entry->samples[slot * 2u];
            right += entry->samples[slot * 2u + 1u];
            any = true;

            entry->resample_phase = 0;
            while (entry->queued_frames > 0) {
                entry->read_frame = (entry->read_frame + 1u) % PSP_AUDIO_MIX_BUFFER_FRAMES;
                entry->queued_frames--;
            }
        }

        if (!any) {
            for (uint32_t rest = frame; rest < max_frames; ++rest) {
                stereo[rest * 2u] = 0;
                stereo[rest * 2u + 1u] = 0;
            }
            return frame;
        }

        stereo[frame * 2u] = clamp_sample(left);
        stereo[frame * 2u + 1u] = clamp_sample(right);
    }

    return max_frames;
}

static bool handle_reserve(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    (void)memory;
    if (!cpu || !active_audio || code != PSP_SYSCALL_AUDIO_CH_RESERVE) return false;
    cpu->gpr[2] = (uint32_t)psp_audio_hle_reserve(active_audio,
        (int32_t)cpu->gpr[4], cpu->gpr[5], cpu->gpr[6]);
    return true;
}

static bool handle_release(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    (void)memory;
    if (!cpu || !active_audio || code != PSP_SYSCALL_AUDIO_CH_RELEASE) return false;
    cpu->gpr[2] = (uint32_t)psp_audio_hle_release(active_audio, (int32_t)cpu->gpr[4]);
    return true;
}

static bool handle_output(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    if (!cpu || !memory || !active_audio || code != PSP_SYSCALL_AUDIO_OUTPUT_BLOCKING) return false;
    int32_t result = psp_audio_hle_output(active_audio, memory,
        (int32_t)cpu->gpr[4], cpu->gpr[5], cpu->gpr[5], cpu->gpr[6]);
    cpu->gpr[2] = result < 0 ? (uint32_t)result :
        active_audio->channels[cpu->gpr[4]].sample_count;
    return true;
}

static bool handle_panned_output(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    if (!cpu || !memory || !active_audio || code != PSP_SYSCALL_AUDIO_OUTPUT_PANNED_BLOCKING) return false;
    int32_t result = psp_audio_hle_output(active_audio, memory,
        (int32_t)cpu->gpr[4], cpu->gpr[5], cpu->gpr[6], cpu->gpr[7]);
    cpu->gpr[2] = result < 0 ? (uint32_t)result :
        active_audio->channels[cpu->gpr[4]].sample_count;
    return true;
}

static bool handle_get_rest(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    (void)memory;
    if (!cpu || !active_audio || code != PSP_SYSCALL_AUDIO_GET_REST_LENGTH) return false;
    cpu->gpr[2] = (uint32_t)psp_audio_hle_get_rest(active_audio, (int32_t)cpu->gpr[4]);
    return true;
}

static bool handle_set_length(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    (void)memory;
    if (!cpu || !active_audio || code != PSP_SYSCALL_AUDIO_SET_DATA_LENGTH) return false;
    cpu->gpr[2] = (uint32_t)psp_audio_hle_set_length(active_audio,
        (int32_t)cpu->gpr[4], cpu->gpr[5]);
    return true;
}

static bool handle_set_format(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    (void)memory;
    if (!cpu || !active_audio || code != PSP_SYSCALL_AUDIO_CHANGE_CONFIG) return false;
    cpu->gpr[2] = (uint32_t)psp_audio_hle_set_format(active_audio,
        (int32_t)cpu->gpr[4], cpu->gpr[5]);
    return true;
}

static bool handle_set_volume(PSP_CPU *cpu, PSP_Memory *memory, uint32_t code)
{
    (void)memory;
    if (!cpu || !active_audio || code != PSP_SYSCALL_AUDIO_CHANGE_VOLUME) return false;
    cpu->gpr[2] = (uint32_t)psp_audio_hle_set_volume(active_audio,
        (int32_t)cpu->gpr[4], cpu->gpr[5], cpu->gpr[6]);
    return true;
}

bool psp_audio_hle_register(PSP_Audio_HLE *audio)
{
    if (!audio) return false;
    active_audio = audio;
    if (!psp_syscall_register(PSP_SYSCALL_AUDIO_CH_RESERVE, handle_reserve) ||
        !psp_syscall_register(PSP_SYSCALL_AUDIO_CH_RELEASE, handle_release) ||
        !psp_syscall_register(PSP_SYSCALL_AUDIO_OUTPUT_BLOCKING, handle_output) ||
        !psp_syscall_register(PSP_SYSCALL_AUDIO_OUTPUT_PANNED_BLOCKING, handle_panned_output) ||
        !psp_syscall_register(PSP_SYSCALL_AUDIO_GET_REST_LENGTH, handle_get_rest) ||
        !psp_syscall_register(PSP_SYSCALL_AUDIO_SET_DATA_LENGTH, handle_set_length) ||
        !psp_syscall_register(PSP_SYSCALL_AUDIO_CHANGE_CONFIG, handle_set_format) ||
        !psp_syscall_register(PSP_SYSCALL_AUDIO_CHANGE_VOLUME, handle_set_volume) ||
        !psp_nid_register(PSP_NID_AUDIO_CH_RESERVE, PSP_SYSCALL_AUDIO_CH_RESERVE) ||
        !psp_nid_register(PSP_NID_AUDIO_CH_RELEASE, PSP_SYSCALL_AUDIO_CH_RELEASE) ||
        !psp_nid_register(PSP_NID_AUDIO_OUTPUT_BLOCKING, PSP_SYSCALL_AUDIO_OUTPUT_BLOCKING) ||
        !psp_nid_register(PSP_NID_AUDIO_OUTPUT_PANNED_BLOCKING, PSP_SYSCALL_AUDIO_OUTPUT_PANNED_BLOCKING) ||
        !psp_nid_register(PSP_NID_AUDIO_GET_REST_LENGTH, PSP_SYSCALL_AUDIO_GET_REST_LENGTH) ||
        !psp_nid_register(PSP_NID_AUDIO_SET_DATA_LENGTH, PSP_SYSCALL_AUDIO_SET_DATA_LENGTH) ||
        !psp_nid_register(PSP_NID_AUDIO_CHANGE_CONFIG, PSP_SYSCALL_AUDIO_CHANGE_CONFIG) ||
        !psp_nid_register(PSP_NID_AUDIO_CHANGE_VOLUME, PSP_SYSCALL_AUDIO_CHANGE_VOLUME)) {
        psp_audio_hle_unregister();
        return false;
    }
    return true;
}

void psp_audio_hle_unregister(void)
{
    psp_syscall_unregister(PSP_SYSCALL_AUDIO_CH_RESERVE);
    psp_syscall_unregister(PSP_SYSCALL_AUDIO_CH_RELEASE);
    psp_syscall_unregister(PSP_SYSCALL_AUDIO_OUTPUT_BLOCKING);
    psp_syscall_unregister(PSP_SYSCALL_AUDIO_OUTPUT_PANNED_BLOCKING);
    psp_syscall_unregister(PSP_SYSCALL_AUDIO_GET_REST_LENGTH);
    psp_syscall_unregister(PSP_SYSCALL_AUDIO_SET_DATA_LENGTH);
    psp_syscall_unregister(PSP_SYSCALL_AUDIO_CHANGE_CONFIG);
    psp_syscall_unregister(PSP_SYSCALL_AUDIO_CHANGE_VOLUME);
    psp_nid_unregister(PSP_NID_AUDIO_CH_RESERVE);
    psp_nid_unregister(PSP_NID_AUDIO_CH_RELEASE);
    psp_nid_unregister(PSP_NID_AUDIO_OUTPUT_BLOCKING);
    psp_nid_unregister(PSP_NID_AUDIO_OUTPUT_PANNED_BLOCKING);
    psp_nid_unregister(PSP_NID_AUDIO_GET_REST_LENGTH);
    psp_nid_unregister(PSP_NID_AUDIO_SET_DATA_LENGTH);
    psp_nid_unregister(PSP_NID_AUDIO_CHANGE_CONFIG);
    psp_nid_unregister(PSP_NID_AUDIO_CHANGE_VOLUME);
    active_audio = NULL;
}
