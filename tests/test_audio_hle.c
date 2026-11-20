#include "audio/psp_audio_hle.h"
#include "cpu/psp_syscall.h"
#include "loader/psp_nid.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

static void write_s16(PSP_Memory *memory, uint32_t address, int16_t value)
{
    uint16_t raw = (uint16_t)value;
    psp_memory_write8(memory, address, (uint8_t)raw);
    psp_memory_write8(memory, address + 1u, (uint8_t)(raw >> 8));
}

int main(void)
{
    PSP_Memory memory = {0};
    PSP_Audio_HLE audio;
    PSP_CPU cpu;
    memset(&cpu, 0, sizeof(cpu));
    assert(psp_memory_init(&memory));
    psp_audio_hle_init(&audio);

    int32_t ch0 = psp_audio_hle_reserve(&audio, -1, 64, 0x10);
    int32_t ch1 = psp_audio_hle_reserve(&audio, -1, 64, 0x10);
    assert(ch0 == 0 && ch1 == 1);
    assert(psp_audio_hle_reserve(&audio, ch0, 64, 0x10) == -1);
    assert(psp_audio_hle_reserve(&audio, -1, 4, 0x10) == -1);
    assert(psp_audio_hle_reserve(&audio, -1, 64, 0x20) == -1);

    uint32_t pcm0 = PSP_RAM_BASE + 0x100u;
    uint32_t pcm1 = PSP_RAM_BASE + 0x300u;
    write_s16(&memory, pcm0, 10000);
    write_s16(&memory, pcm0 + 2u, -10000);
    write_s16(&memory, pcm0 + 4u, 20000);
    write_s16(&memory, pcm0 + 6u, -20000);
    write_s16(&memory, pcm1, 10000);
    write_s16(&memory, pcm1 + 2u, 10000);
    write_s16(&memory, pcm1 + 4u, 10000);
    write_s16(&memory, pcm1 + 6u, 10000);

    assert(psp_audio_hle_output(&audio, &memory, ch0, 0x8000, 0x4000, pcm0) == 0);
    assert(psp_audio_hle_output(&audio, &memory, ch1, 0x4000, 0x8000, pcm1) == 0);
    assert(psp_audio_hle_output(&audio, &memory, ch1, 0x8000, 0x8000,
                                 PSP_RAM_BASE + PSP_RAM_SIZE - 2u) == -1);

    int16_t mixed[8];
    assert(psp_audio_hle_read(&audio, mixed, 4) == 4);
    assert(mixed[0] == 15000 && mixed[1] == 15000);
    assert(mixed[2] == -5000 && mixed[3] == 5000);
    assert(mixed[4] == 25000 && mixed[5] == 15000);
    assert(mixed[6] == -15000 && mixed[7] == -5000);
    assert(psp_audio_hle_get_rest(&audio, ch0) == 60);
    assert(psp_audio_hle_set_length(&audio, ch0, 128) == 0);
    assert(psp_audio_hle_set_length(&audio, ch0, 65) == -1);
    assert(psp_audio_hle_set_volume(&audio, ch0, 0x4000, 0x8000) == 0);
    assert(psp_audio_hle_set_format(&audio, ch0, 0) == 0);
    assert(psp_audio_hle_set_format(&audio, ch0, 0x20) == -1);
    assert(psp_audio_hle_release(&audio, ch0) == 0);
    assert(psp_audio_hle_release(&audio, ch1) == 0);
    assert(psp_audio_hle_release(&audio, ch1) == -1);

    int32_t stereo = psp_audio_hle_reserve(&audio, -1, 64, 0);
    assert(stereo == 0);
    write_s16(&memory, pcm0, 1200);
    write_s16(&memory, pcm0 + 2u, -1200);
    write_s16(&memory, pcm0 + 4u, 2400);
    write_s16(&memory, pcm0 + 6u, -2400);
    assert(psp_audio_hle_output(&audio, &memory, stereo, 0x8000, 0x4000, pcm0) == 0);
    int16_t stereo_out[4];
    assert(psp_audio_hle_read(&audio, stereo_out, 2) == 2);
    assert(stereo_out[0] == 1200 && stereo_out[1] == -600);
    assert(stereo_out[2] == 2400 && stereo_out[3] == -1200);

    psp_syscall_reset();
    psp_nid_reset();
    psp_audio_hle_reset(&audio);
    assert(psp_audio_hle_register(&audio));
    uint32_t syscall_code = 0;
    assert(psp_nid_resolve(PSP_NID_AUDIO_CH_RESERVE, &syscall_code));
    assert(syscall_code == PSP_SYSCALL_AUDIO_CH_RESERVE);
    cpu.gpr[4] = (uint32_t)-1;
    cpu.gpr[5] = 64;
    cpu.gpr[6] = 0x10;
    assert(psp_syscall_dispatch(&cpu, &memory, syscall_code));
    assert(cpu.gpr[2] == 0u);

    assert(psp_nid_resolve(PSP_NID_AUDIO_OUTPUT, &syscall_code));
    cpu.gpr[4] = 0;
    cpu.gpr[5] = 0x8000;
    cpu.gpr[6] = pcm0;
    assert(psp_syscall_dispatch(&cpu, &memory, syscall_code));
    assert(cpu.gpr[2] == 0u);

    assert(psp_nid_resolve(PSP_NID_AUDIO_OUTPUT_BLOCKING, &syscall_code));
    cpu.gpr[4] = 0;
    cpu.gpr[5] = 0x8000;
    cpu.gpr[6] = pcm0;
    assert(psp_syscall_dispatch(&cpu, &memory, syscall_code));
    assert(cpu.gpr[2] == 64u);

    assert(psp_nid_resolve(PSP_NID_AUDIO_GET_REST_LENGTH, &syscall_code));
    cpu.gpr[4] = 0;
    assert(psp_syscall_dispatch(&cpu, &memory, syscall_code));
    assert(cpu.gpr[2] == 64u);

    assert(psp_nid_resolve(PSP_NID_AUDIO_SET_DATA_LENGTH, &syscall_code));
    cpu.gpr[4] = 0;
    cpu.gpr[5] = 128;
    assert(psp_syscall_dispatch(&cpu, &memory, syscall_code));
    assert(cpu.gpr[2] == 0u);

    assert(psp_nid_resolve(PSP_NID_AUDIO_CHANGE_CONFIG, &syscall_code));
    cpu.gpr[4] = 0;
    cpu.gpr[5] = 0;
    assert(psp_syscall_dispatch(&cpu, &memory, syscall_code));
    assert(cpu.gpr[2] == 0u);

    assert(psp_nid_resolve(PSP_NID_AUDIO_CHANGE_VOLUME, &syscall_code));
    cpu.gpr[4] = 0;
    cpu.gpr[5] = 0x4000;
    cpu.gpr[6] = 0x4000;
    assert(psp_syscall_dispatch(&cpu, &memory, syscall_code));
    assert(cpu.gpr[2] == 0u);

    assert(psp_nid_resolve(PSP_NID_AUDIO_OUTPUT_PANNED_BLOCKING, &syscall_code));
    cpu.gpr[4] = 0;
    cpu.gpr[5] = 0x8000;
    cpu.gpr[6] = 0x8000;
    cpu.gpr[7] = pcm0;
    assert(psp_syscall_dispatch(&cpu, &memory, syscall_code));
    assert(cpu.gpr[2] == 128u);

    psp_audio_hle_unregister();
    psp_syscall_reset();
    psp_nid_reset();
    psp_memory_destroy(&memory);
    return 0;
}
