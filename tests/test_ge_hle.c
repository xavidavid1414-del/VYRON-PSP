#include "psp_ge_hle.h"
#include "../cpu/psp_syscall.h"
#include "../loader/psp_nid.h"
#include "../memory/psp_memory.h"
#include "../video/psp_video.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void command(PSP_Memory *memory, uint32_t address,
                    uint8_t opcode, uint32_t value)
{
    psp_memory_write32(memory, address,
        ((uint32_t)opcode << 24) | (value & 0x00FFFFFFu));
}

int main(void)
{
    PSP_Memory memory;
    PSP_Video video;
    PSP_GE ge;
    PSP_GE_HLE service;
    PSP_CPU cpu = {0};

    assert(psp_memory_init(&memory));
    assert(psp_video_init(&video));
    assert(psp_ge_init(&ge, &memory, &video));
    psp_ge_hle_init(&service, &ge);
    psp_syscall_reset();
    psp_nid_reset();
    assert(psp_ge_hle_register(&service));

    uint32_t list = PSP_RAM_BASE + 0x100u;
    command(&memory, list, 0x0C, 0);
    cpu.gpr[4] = list;
    cpu.gpr[5] = 0;
    cpu.gpr[6] = 0xFFFFFFFFu;
    cpu.gpr[7] = 0;
    uint32_t syscall_code = 0;
    assert(psp_nid_resolve(0xAB49E76Au, &syscall_code));
    assert(syscall_code == 10u);
    assert(psp_syscall_dispatch(&cpu, &memory, 10u));
    uint32_t id = cpu.gpr[2];
    assert(id == 0x35000000u);

    cpu.gpr[4] = id;
    cpu.gpr[5] = 1;
    assert(psp_syscall_dispatch(&cpu, &memory, 13u));
    assert(cpu.gpr[2] == 0);

    cpu.gpr[4] = 0;
    assert(psp_syscall_dispatch(&cpu, &memory, 14u));
    assert(cpu.gpr[2] == 0);

    cpu.gpr[4] = id;
    assert(psp_syscall_dispatch(&cpu, &memory, 15u));
    assert(cpu.gpr[2] == 0);
    assert(psp_syscall_dispatch(&cpu, &memory, 13u));
    assert(cpu.gpr[2] == 0x80020190u);

    /* Enqueueing a real PSP GE list reaches the software renderer. */
    const uint32_t draw_list = PSP_RAM_BASE + 0x200u;
    const uint32_t vertex = PSP_RAM_BASE + 0x400u;
    psp_video_clear(&video, 0);
    psp_memory_write32(&memory, vertex, 0xFFFF0000u);
    psp_memory_write8(&memory, vertex + 4u, 10u);
    psp_memory_write8(&memory, vertex + 5u, 0u);
    psp_memory_write8(&memory, vertex + 6u, 10u);
    psp_memory_write8(&memory, vertex + 7u, 0u);
    command(&memory, draw_list, 0x10, 0x00080000u);
    command(&memory, draw_list + 4u, 0x01, 0x400u);
    command(&memory, draw_list + 8u, 0x12,
            (7u << 2) | (2u << 7) | (1u << 23));
    command(&memory, draw_list + 12u, 0x04, 1u);
    command(&memory, draw_list + 16u, 0x0C, 0);
    cpu.gpr[4] = draw_list;
    cpu.gpr[5] = 0;
    cpu.gpr[6] = 0xFFFFFFFFu;
    cpu.gpr[7] = 0;
    assert(psp_syscall_dispatch(&cpu, &memory, 10u));
    assert(cpu.gpr[2] == 0x35000000u);
    assert(psp_video_get_pixel(&video, 10, 10) == 0xFFFF0000u);
    cpu.gpr[4] = cpu.gpr[2];
    assert(psp_syscall_dispatch(&cpu, &memory, 15u));
    assert(cpu.gpr[2] == 0);

    psp_ge_hle_unregister();
    psp_syscall_reset();
    psp_nid_reset();
    psp_ge_destroy(&ge);
    psp_video_destroy(&video);
    psp_memory_destroy(&memory);
    puts("PSP GE HLE tests passed");
    return 0;
}
