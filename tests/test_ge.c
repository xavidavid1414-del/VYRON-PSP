#include "psp_ge.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void put16(PSP_Memory *memory, uint32_t address, uint16_t value)
{
    psp_memory_write8(memory, address, (uint8_t)value);
    psp_memory_write8(memory, address + 1, (uint8_t)(value >> 8));
}

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
    assert(psp_memory_init(&memory));
    assert(psp_video_init(&video));
    assert(psp_ge_init(&ge, &memory, &video));

    const uint32_t list = PSP_RAM_BASE + 0x100;
    const uint32_t vertices = PSP_RAM_BASE + 0x1000;
    const uint32_t vtype = (4u << 2) | (2u << 7) | (1u << 23);

    /* Three colored 2D vertices form a filled triangle. */
    psp_memory_write32(&memory, vertices, 0xFFFF0000u);
    put16(&memory, vertices + 4, 10); put16(&memory, vertices + 6, 10);
    psp_memory_write32(&memory, vertices + 8, 0xFFFF0000u);
    put16(&memory, vertices + 12, 40); put16(&memory, vertices + 14, 10);
    psp_memory_write32(&memory, vertices + 16, 0xFFFF0000u);
    put16(&memory, vertices + 20, 10); put16(&memory, vertices + 22, 40);

    command(&memory, list, 0x10, 0x00080000u); /* BASE = 0x08000000 */
    command(&memory, list + 4, 0x01, 0x1000u); /* VADDR */
    command(&memory, list + 8, 0x12, vtype); /* VTYPE */
    command(&memory, list + 12, 0x04, (3u << 16) | 3u);
    command(&memory, list + 16, 0x0C, 0); /* END */

    assert(psp_ge_execute_list(&ge, list, 8));
    assert(ge.commands_executed == 5);
    assert(ge.primitives_drawn == 1);
    assert(psp_video_get_pixel(&video, 15, 15) == 0xFFFF0000u);
    assert(psp_video_get_pixel(&video, 35, 35) == 0);

    /* Invalid vertex memory is rejected instead of reading out of bounds. */
    psp_ge_reset(&ge);
    command(&memory, list + 8, 0x01, 0xFFFFF0u);
    command(&memory, list + 12, 0x12, vtype);
    command(&memory, list + 16, 0x04, (3u << 16) | 3u);
    command(&memory, list + 20, 0x0C, 0);
    assert(!psp_ge_execute_list(&ge, list + 8, 8));

    /* A list without END cannot run past its caller-supplied command budget. */
    psp_ge_reset(&ge);
    command(&memory, list + 32, 0x12, vtype);
    assert(!psp_ge_execute_list(&ge, list + 32, 2));

    psp_video_destroy(&video);
    psp_memory_destroy(&memory);
    puts("PSP GE tests passed");
    return 0;
}
