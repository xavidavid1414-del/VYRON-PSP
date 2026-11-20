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

    int32_t ch0 = psp_audio_hle_reserve(&audio, -1, 4, 44100);
    int32_t ch1 = psp_audio_hle_reserve(&audio, -1, 4, 44100);
    assert(ch0 == 0 && ch1 == 1);
    assert(psp_audio_hle_reserve(&audio, ch0, 4, 44100) == -1);
    assert(psp_audio_hle_reserve(&audio, -1, 0, 44100) == -1);
    assert(psp_audio_hle_reserve(&audio, -1, 4, 48000) == -1);

    uint32_t pcm0 = PSP_RAM_BASE + 0x100u;
    uint32_t pcm1 = PSP_RAM_BASE + 0x200u;
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
    assert(mixed[0] == 15000);
    assert(mixed[1] == 15000);
    assert(mixed[2] == -5000);
    assert(mixed[3] == 5000);
    assert(mixed[4] == 15000);
    assert(mixed[5] == 15000);
    assert(mixed[6] == -5000);
    assert(mixed[7] == 5000);

    assert(psp_audio_hle_release(&audio, ch0) == 0);
    assert(psp_audio_hle_release(&audio, ch1) == 0);
    assert(psp_audio_hle_release(&audio, ch1) == -1);

    int32_t slow = psp_audio_hle_reserve(&audio, -1, 2, 22050);
    assert(slow == 0);
    write_s16(&memory, pcm0, 1000);
    write_s16(&memory, pcm0 + 2u, 2000);
    assert(psp_audio_hle_output(&audio, &memory, slow, 0x8000, 0x8000, pcm0) == 0);
    int16_t resampled[8];
    assert(psp_audio_hle_read(&audio, resampled, 4) == 4);
    assert(resampled[0] == 1000 && resampled[2] == 1000);
    assert(resampled[4] == 2000 && resampled[6] == 2000);

    psp_syscall_reset();
    psp_nid_reset();
    psp_audio_hle_reset(&audio);
    assert(psp_audio_hle_register(&audio));
    uint32_t syscall_code = 0;
    assert(psp_nid_resolve(PSP_NID_AUDIO_CH_RESERVE, &syscall_code));
    assert(syscall_code == PSP_SYSCALL_AUDIO_CH_RESERVE);
    cpu.gpr[4] = (uint32_t)-1;
    cpu.gpr[5] = 64;
    cpu.gpr[6] = 44100;
    assert(psp_syscall_dispatch(&cpu, &memory, syscall_code));
    assert(cpu.gpr[2] == 0u);

    assert(psp_nid_resolve(PSP_NID_AUDIO_OUTPUT_PANNED_BLOCKING, &syscall_code));
    assert(syscall_code == PSP_SYSCALL_AUDIO_OUTPUT_PANNED_BLOCKING);
    cpu.gpr[4] = 0;
    cpu.gpr[5] = 0x8000;
    cpu.gpr[6] = 0x8000;
    cpu.gpr[7] = pcm0;
    assert(psp_syscall_dispatch(&cpu, &memory, syscall_code));
    assert((int32_t)cpu.gpr[2] == -1); /* source range is intentionally invalid for 64 frames */

    psp_audio_hle_unregister();
    psp_syscall_reset();
    psp_nid_reset();
    psp_memory_destroy(&memory);
    return 0;
}
