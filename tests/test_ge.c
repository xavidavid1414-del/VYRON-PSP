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

static void put_float(PSP_Memory *memory, uint32_t address, float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    psp_memory_write32(memory, address, bits);
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

    /* Textured triangles sample 8888 texels with interpolated UVs. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    const uint32_t texture = PSP_RAM_BASE + 0x2000;
    const uint32_t textured_vertices = PSP_RAM_BASE + 0x1400;
    psp_memory_write32(&memory, texture, 0xFFFF0000u);
    psp_memory_write32(&memory, texture + 4, 0xFF00FF00u);
    psp_memory_write32(&memory, texture + 8, 0xFF0000FFu);
    psp_memory_write32(&memory, texture + 12, 0xFFFFFFFFu);
    const uint32_t textured_vtype =
        3u | (4u << 2) | (2u << 7) | (1u << 23);
    for (uint32_t i = 0; i < 3; i++) {
        uint32_t address = textured_vertices + i * 16u;
        put_float(&memory, address, i == 1 ? 2.0f : 0.0f);
        put_float(&memory, address + 4, i == 2 ? 2.0f : 0.0f);
        psp_memory_write32(&memory, address + 8, 0xFFFFFFFFu);
        put16(&memory, address + 12, i == 1 ? 40 : 10);
        put16(&memory, address + 14, i == 2 ? 40 : 10);
    }
    command(&memory, list + 128, 0x10, 0x00080000u);
    command(&memory, list + 132, 0xA0, 0x2000u);
    command(&memory, list + 136, 0xA8, 0x00080002u);
    command(&memory, list + 140, 0xB8, 0x00000101u);
    command(&memory, list + 144, 0xC3, 3u);
    command(&memory, list + 148, 0xC7, 0x00000101u);
    command(&memory, list + 152, 0x1E, 1u);
    command(&memory, list + 156, 0x01, 0x1400u);
    command(&memory, list + 160, 0x12, textured_vtype);
    command(&memory, list + 164, 0x04, (3u << 16) | 3u);
    command(&memory, list + 168, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 128, 12));
    assert(psp_video_get_pixel(&video, 15, 15) == 0xFFFF0000u);
    assert(psp_video_get_pixel(&video, 35, 10) == 0xFF00FF00u);

    /* 5650 textures decode to opaque RGB pixels as well. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    psp_memory_write8(&memory, PSP_RAM_BASE + 0x2100, 0x00);
    psp_memory_write8(&memory, PSP_RAM_BASE + 0x2101, 0xF8);
    psp_memory_write8(&memory, PSP_RAM_BASE + 0x2102, 0xE0);
    psp_memory_write8(&memory, PSP_RAM_BASE + 0x2103, 0x07);
    command(&memory, list + 176, 0x10, 0x00080000u);
    command(&memory, list + 180, 0xA0, 0x2100u);
    command(&memory, list + 184, 0xA8, 0x00080002u);
    command(&memory, list + 188, 0xB8, 0x00000101u);
    command(&memory, list + 192, 0xC3, 0u);
    command(&memory, list + 196, 0xC7, 0x00000101u);
    command(&memory, list + 200, 0x1E, 1u);
    command(&memory, list + 204, 0x01, 0x1400u);
    command(&memory, list + 208, 0x12, textured_vtype);
    command(&memory, list + 212, 0x04, (3u << 16) | 3u);
    command(&memory, list + 216, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 176, 12));
    assert(psp_video_get_pixel(&video, 15, 15) == 0xFFFF0000u);
    assert(psp_video_get_pixel(&video, 35, 10) == 0xFF00FF00u);

    /* 5551 and 4444 texture formats retain alpha and channel decoding. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    put16(&memory, PSP_RAM_BASE + 0x2200, 0xFC00u);
    put16(&memory, PSP_RAM_BASE + 0x2202, 0x83E0u);
    command(&memory, list + 224, 0x10, 0x00080000u);
    command(&memory, list + 228, 0xA0, 0x2200u);
    command(&memory, list + 232, 0xA8, 0x00080002u);
    command(&memory, list + 236, 0xB8, 0x00000101u);
    command(&memory, list + 240, 0xC3, 1u);
    command(&memory, list + 244, 0xC7, 0x00000101u);
    command(&memory, list + 248, 0x1E, 1u);
    command(&memory, list + 252, 0x01, 0x1400u);
    command(&memory, list + 256, 0x12, textured_vtype);
    command(&memory, list + 260, 0x04, (3u << 16) | 3u);
    command(&memory, list + 264, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 224, 12));
    assert(psp_video_get_pixel(&video, 15, 15) == 0xFFFF0000u);
    assert(psp_video_get_pixel(&video, 35, 10) == 0xFF00FF00u);

    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    put16(&memory, PSP_RAM_BASE + 0x2300, 0xFF00u);
    put16(&memory, PSP_RAM_BASE + 0x2302, 0xF0F0u);
    command(&memory, list + 272, 0x10, 0x00080000u);
    command(&memory, list + 276, 0xA0, 0x2300u);
    command(&memory, list + 280, 0xA8, 0x00080002u);
    command(&memory, list + 284, 0xB8, 0x00000101u);
    command(&memory, list + 288, 0xC3, 2u);
    command(&memory, list + 292, 0xC7, 0x00000101u);
    command(&memory, list + 296, 0x1E, 1u);
    command(&memory, list + 300, 0x01, 0x1400u);
    command(&memory, list + 304, 0x12, textured_vtype);
    command(&memory, list + 308, 0x04, (3u << 16) | 3u);
    command(&memory, list + 312, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 272, 12));
    assert(psp_video_get_pixel(&video, 15, 15) == 0xFFFF0000u);
    assert(psp_video_get_pixel(&video, 35, 10) == 0xFF00FF00u);

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
    command(&memory, list + 44, 0xD4, (20u << 10) | 20u);
    command(&memory, list + 48, 0xD5, (25u << 10) | 25u);
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

    /* Alpha blending combines a translucent source with the framebuffer. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0xFF0000FFu);
    psp_memory_write32(&memory, vertices, 0x80FF0000u);
    put16(&memory, vertices + 4, 20);
    put16(&memory, vertices + 6, 20);
    command(&memory, list + 344, 0x10, 0x00080000u);
    command(&memory, list + 348, 0x01, 0x1000u);
    command(&memory, list + 352, 0x12, vtype);
    command(&memory, list + 356, 0x21, 1u);
    command(&memory, list + 360, 0xDF, (3u << 8) | (2u << 4));
    command(&memory, list + 364, 0x04, 1u);
    command(&memory, list + 368, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 344, 8));
    uint32_t blended = psp_video_get_pixel(&video, 20, 20);
    assert(((blended >> 16) & 0xFFu) >= 126u);
    assert((blended & 0xFFu) >= 126u);
    assert(((blended >> 8) & 0xFFu) == 0u);

    psp_memory_write32(&memory, vertices, 0xFFFF0000u);
    put16(&memory, vertices + 4, 10);
    put16(&memory, vertices + 6, 10);

    /* Indexed triangles fetch 8-bit indices from IADDR. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    const uint32_t indices = PSP_RAM_BASE + 0x1800;
    psp_memory_write8(&memory, indices, 2);
    psp_memory_write8(&memory, indices + 1, 0);
    psp_memory_write8(&memory, indices + 2, 1);
    command(&memory, list + 320, 0x10, 0x00080000u);
    command(&memory, list + 324, 0x01, 0x1000u);
    command(&memory, list + 328, 0x02, 0x1800u);
    command(&memory, list + 332, 0x12, vtype | (1u << 11));
    command(&memory, list + 336, 0x04, (3u << 16) | 3u);
    command(&memory, list + 340, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 320, 8));
    assert(ge.primitives_drawn == 1);
    assert(psp_video_get_pixel(&video, 15, 15) != 0);

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
