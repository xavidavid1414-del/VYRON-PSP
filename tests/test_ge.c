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
    psp_memory_write32(&memory, vertices + 8, 0xFF00FF00u);
    put16(&memory, vertices + 12, 40); put16(&memory, vertices + 14, 10);
    psp_memory_write32(&memory, vertices + 16, 0xFF0000FFu);
    put16(&memory, vertices + 20, 10); put16(&memory, vertices + 22, 40);

    command(&memory, list, 0x10, 0x00080000u); /* BASE = 0x08000000 */
    command(&memory, list + 4, 0x01, 0x1000u); /* VADDR */
    command(&memory, list + 8, 0x12, vtype); /* VTYPE */
    command(&memory, list + 12, 0x04, (3u << 16) | 3u);
    command(&memory, list + 16, 0x0C, 0); /* END */

    assert(psp_ge_execute_list(&ge, list, 8));
    assert(ge.commands_executed == 5);
    assert(ge.primitives_drawn == 1);
    uint32_t interpolated = psp_video_get_pixel(&video, 15, 15);
    assert((interpolated >> 24) == 0xFFu);
    assert(((interpolated >> 16) & 0xFFu) > 140u);
    assert(((interpolated >> 8) & 0xFFu) > 25u);
    assert((interpolated & 0xFFu) > 25u);
    assert(psp_video_get_pixel(&video, 35, 35) == 0);

    /* Sprite primitives obey the configured scissor rectangle. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    const uint32_t sprite_vertices = PSP_RAM_BASE + 0x1100;
    psp_memory_write32(&memory, sprite_vertices, 0xFF00FF00u);
    put16(&memory, sprite_vertices + 4, 5);
    put16(&memory, sprite_vertices + 6, 5);
    psp_memory_write32(&memory, sprite_vertices + 8, 0xFF00FF00u);
    put16(&memory, sprite_vertices + 12, 30);
    put16(&memory, sprite_vertices + 14, 30);
    command(&memory, list + 32, 0x10, 0x00080000u);
    command(&memory, list + 36, 0x01, 0x1100u);
    command(&memory, list + 40, 0x12, vtype);
    command(&memory, list + 44, 0x17, (20u << 10) | 20u);
    command(&memory, list + 48, 0x18, (25u << 10) | 25u);
    command(&memory, list + 52, 0x04, (6u << 16) | 2u);
    command(&memory, list + 56, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 32, 8));
    assert(psp_video_get_pixel(&video, 22, 22) == 0xFF00FF00u);
    assert(psp_video_get_pixel(&video, 10, 10) == 0);

    /* Line-strip primitive joins each adjacent vertex pair. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    command(&memory, list + 64, 0x10, 0x00080000u);
    command(&memory, list + 68, 0x01, 0x1000u);
    command(&memory, list + 72, 0x12, vtype);
    command(&memory, list + 76, 0x04, (2u << 16) | 3u);
    command(&memory, list + 80, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 64, 8));
    assert(psp_video_get_pixel(&video, 20, 10) != 0);
    assert(ge.primitives_drawn == 2);

    /* Nested display-list flow supports bounded CALL/RET control flow. */
    psp_ge_reset(&ge);
    command(&memory, list + 96, 0x10, 0x00080000u);
    command(&memory, list + 100, 0x0A, 0x1300u);
    command(&memory, list + 104, 0x0C, 0);
    command(&memory, PSP_RAM_BASE + 0x1300, 0x0B, 0);
    assert(psp_ge_execute_list(&ge, list + 96, 4));
    assert(ge.commands_executed == 4);

    /* Invalid vertex memory is rejected instead of reading out of bounds. */
    psp_ge_reset(&ge);
    command(&memory, list + 8, 0x10, 0x00FF0000u);
    command(&memory, list + 12, 0x01, 0xFFFFF0u);
    command(&memory, list + 16, 0x12, vtype);
    command(&memory, list + 20, 0x04, (3u << 16) | 3u);
    command(&memory, list + 24, 0x0C, 0);
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
