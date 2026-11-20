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

static uint32_t encode_float24(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits >> 8;
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
    const uint32_t vtype = (7u << 2) | (2u << 7) | (1u << 23);

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

    /* Stencil failures update the framebuffer alpha channel and reject RGB. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    psp_video_put_pixel(&video, 15, 15, 0xFF112233u);
    psp_memory_write32(&memory, PSP_VRAM_BASE +
        ((uint32_t)15u * PSP_VIDEO_STRIDE + 15u) * 4u, 0xFF112233u);
    command(&memory, list + 64, 0x10, 0x00080000u);
    command(&memory, list + 68, 0x01, 0x1000u);
    command(&memory, list + 72, 0x12, vtype);
    command(&memory, list + 76, 0x24, 1u); /* Stencil enable */
    command(&memory, list + 80, 0xDC, (0xFFu << 16) | 0u); /* Never */
    command(&memory, list + 84, 0xDD, 1u << 8); /* Stencil fail: zero */
    command(&memory, list + 88, 0x04, (3u << 16) | 3u);
    command(&memory, list + 92, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 64, 10));
    assert(psp_video_get_pixel(&video, 15, 15) == 0x00112233u);

    /* Passing stencil can replace alpha with the reference value. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    psp_memory_write32(&memory, PSP_VRAM_BASE +
        ((uint32_t)15u * PSP_VIDEO_STRIDE + 15u) * 4u, 0u);
    command(&memory, list + 1280, 0x10, 0x00080000u);
    command(&memory, list + 1284, 0x01, 0x1000u);
    command(&memory, list + 1288, 0x12, vtype);
    command(&memory, list + 1292, 0x24, 1u);
    command(&memory, list + 1296, 0xDC, (0xFFu << 16) | (0x5Au << 8) | 1u);
    command(&memory, list + 1300, 0xDD, 2u); /* Stencil pass: replace */
    command(&memory, list + 1304, 0x04, (3u << 16) | 3u);
    command(&memory, list + 1308, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 1280, 10));
    assert((psp_video_get_pixel(&video, 15, 15) >> 24) == 0x5Au);

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
        3u | (7u << 2) | (2u << 7) | (1u << 23);
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

    /* Minification selects the matching PSP mip level (LOD 2). */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    for (uint32_t i = 0; i < 16u; i++)
        psp_memory_write32(&memory, PSP_RAM_BASE + 0x2800u + i * 4u,
                           0xFFFF0000u);
    for (uint32_t i = 0; i < 4u; i++)
        psp_memory_write32(&memory, PSP_RAM_BASE + 0x2840u + i * 4u,
                           0xFF00FF00u);
    psp_memory_write32(&memory, PSP_RAM_BASE + 0x2850u, 0xFF0000FFu);
    put_float(&memory, textured_vertices, 0.0f);
    put_float(&memory, textured_vertices + 4u, 0.0f);
    put_float(&memory, textured_vertices + 16u, 128.0f);
    put_float(&memory, textured_vertices + 20u, 0.0f);
    put_float(&memory, textured_vertices + 32u, 0.0f);
    put_float(&memory, textured_vertices + 36u, 128.0f);
    command(&memory, list + 1400u, 0x10, 0x00080000u);
    command(&memory, list + 1404u, 0xA0, 0x2800u);
    command(&memory, list + 1408u, 0xA8, 0x00080004u);
    command(&memory, list + 1412u, 0xB8, 0x00000202u);
    command(&memory, list + 1416u, 0xA1, 0x2840u);
    command(&memory, list + 1420u, 0xA9, 0x00080002u);
    command(&memory, list + 1424u, 0xB9, 0x00000101u);
    command(&memory, list + 1428u, 0xA2, 0x2850u);
    command(&memory, list + 1432u, 0xAA, 0x00080001u);
    command(&memory, list + 1436u, 0xBA, 0u);
    command(&memory, list + 1440u, 0xC3, 3u);
    command(&memory, list + 1444u, 0xC6, 2u);
    command(&memory, list + 1448u, 0xC7, 0x00000101u);
    command(&memory, list + 1452u, 0x1E, 1u);
    command(&memory, list + 1456u, 0x01, 0x1400u);
    command(&memory, list + 1460u, 0x12, textured_vtype);
    command(&memory, list + 1464u, 0x04, (3u << 16) | 3u);
    command(&memory, list + 1468u, 0x0C, 0u);
    assert(psp_ge_execute_list(&ge, list + 1400u, 18u));
    assert(psp_video_get_pixel(&video, 15, 15) == 0xFF0000FFu);

    /* Restore the shared UVs after the mipmap minification test. */
    put_float(&memory, textured_vertices, 0.0f);
    put_float(&memory, textured_vertices + 4u, 0.0f);
    put_float(&memory, textured_vertices + 16u, 2.0f);
    put_float(&memory, textured_vertices + 20u, 0.0f);
    put_float(&memory, textured_vertices + 32u, 0.0f);
    put_float(&memory, textured_vertices + 36u, 2.0f);

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

    /* Texture REPLACE ignores the interpolated vertex RGB when TCC is RGBA. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    psp_memory_write32(&memory, texture, 0xFFFF0000u);
    for (uint32_t i = 0; i < 3; i++)
        psp_memory_write32(&memory, textured_vertices + i * 16u + 8u,
                           0xFF000000u);
    command(&memory, list + 480, 0x10, 0x00080000u);
    command(&memory, list + 484, 0xA0, 0x2000u);
    command(&memory, list + 488, 0xA8, 0x00080002u);
    command(&memory, list + 492, 0xB8, 0x00000101u);
    command(&memory, list + 496, 0xC3, 3u);
    command(&memory, list + 500, 0xC7, 0x00000101u);
    command(&memory, list + 504, 0xC9, 3u | (1u << 8));
    command(&memory, list + 508, 0x1E, 1u);
    command(&memory, list + 512, 0x01, 0x1400u);
    command(&memory, list + 516, 0x12, textured_vtype);
    command(&memory, list + 520, 0x04, (3u << 16) | 3u);
    command(&memory, list + 524, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 480, 12));
    assert(psp_video_get_pixel(&video, 15, 15) == 0xFFFF0000u);

    /* Linear filtering interpolates the four neighboring 8888 texels. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    psp_memory_write32(&memory, texture, 0xFFFF0000u);
    psp_memory_write32(&memory, texture + 4u, 0xFF00FF00u);
    psp_memory_write32(&memory, texture + 8u, 0xFF0000FFu);
    psp_memory_write32(&memory, texture + 12u, 0xFFFFFFFFu);
    for (uint32_t i = 0; i < 3; i++) {
        put_float(&memory, textured_vertices + i * 16u, 1.0f);
        put_float(&memory, textured_vertices + i * 16u + 4u, 1.0f);
        psp_memory_write32(&memory, textured_vertices + i * 16u + 8u,
                           0xFFFFFFFFu);
    }
    command(&memory, list + 528, 0x10, 0x00080000u);
    command(&memory, list + 532, 0xA0, 0x2000u);
    command(&memory, list + 536, 0xA8, 0x00080002u);
    command(&memory, list + 540, 0xB8, 0x00000101u);
    command(&memory, list + 544, 0xC3, 3u);
    command(&memory, list + 548, 0xC6, 0x00000101u);
    command(&memory, list + 552, 0xC7, 0x00000101u);
    command(&memory, list + 556, 0x1E, 1u);
    command(&memory, list + 560, 0x01, 0x1400u);
    command(&memory, list + 564, 0x12, textured_vtype);
    command(&memory, list + 568, 0x04, (3u << 16) | 3u);
    command(&memory, list + 572, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 528, 12));
    uint32_t filtered = psp_video_get_pixel(&video, 15, 15);
    assert(((filtered >> 16) & 0xFFu) >= 126u &&
           ((filtered >> 16) & 0xFFu) <= 129u);
    assert(((filtered >> 8) & 0xFFu) >= 126u &&
           ((filtered >> 8) & 0xFFu) <= 129u);
    assert((filtered & 0xFFu) >= 126u &&
           (filtered & 0xFFu) <= 129u);

    /* Front-face culling follows the configured clockwise/counter-clockwise mode. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    command(&memory, list + 736, 0x10, 0x00080000u);
    command(&memory, list + 740, 0x01, 0x1000u);
    command(&memory, list + 744, 0x12, vtype);
    command(&memory, list + 748, 0x1D, 1u);
    command(&memory, list + 752, 0x9B, 1u);
    command(&memory, list + 756, 0x04, (3u << 16) | 3u);
    command(&memory, list + 760, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 736, 8));
    assert(psp_video_get_pixel(&video, 15, 15) == 0);
    command(&memory, list + 764, 0x9B, 0u);
    command(&memory, list + 768, 0x04, (3u << 16) | 3u);
    command(&memory, list + 772, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 764, 4));
    assert(psp_video_get_pixel(&video, 15, 15) != 0);

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
    psp_memory_write32(&memory, PSP_VRAM_BASE +
        ((uint32_t)20u * PSP_VIDEO_STRIDE + 20u) * 4u, 0xFF0000FFu);
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

    /* Alpha testing rejects fragments below the configured threshold. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0xFF0000FFu);
    psp_memory_write32(&memory, vertices, 0x40FF0000u);
    put16(&memory, vertices + 4, 24);
    put16(&memory, vertices + 6, 24);
    command(&memory, list + 372, 0x10, 0x00080000u);
    command(&memory, list + 376, 0x01, 0x1000u);
    command(&memory, list + 380, 0x12, vtype);
    command(&memory, list + 384, 0xDB, (0xFFu << 16) | (0x80u << 8) | 6u);
    command(&memory, list + 388, 0x22, 1u);
    command(&memory, list + 392, 0x04, 1u);
    command(&memory, list + 396, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 372, 8));
    assert(psp_video_get_pixel(&video, 24, 24) == 0xFF0000FFu);

    command(&memory, list + 400, 0xDB, (0xFFu << 16) | (0x20u << 8) | 6u);
    command(&memory, list + 404, 0x04, 1u);
    command(&memory, list + 408, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 400, 4));
    assert(psp_video_get_pixel(&video, 24, 24) == 0x40FF0000u);

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

    /* 16-bit and 32-bit-stride indices use their low 16-bit vertex index. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    put16(&memory, indices, 2);
    put16(&memory, indices + 2, 0);
    put16(&memory, indices + 4, 1);
    command(&memory, list + 576, 0x10, 0x00080000u);
    command(&memory, list + 580, 0x01, 0x1000u);
    command(&memory, list + 584, 0x02, 0x1800u);
    command(&memory, list + 588, 0x12, vtype | (2u << 11));
    command(&memory, list + 592, 0x04, (3u << 16) | 3u);
    command(&memory, list + 596, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 576, 8));
    assert(psp_video_get_pixel(&video, 15, 15) != 0);

    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    psp_memory_write32(&memory, indices, 0xABCD0002u);
    psp_memory_write32(&memory, indices + 4u, 0x12340000u);
    psp_memory_write32(&memory, indices + 8u, 0xFFFF0001u);
    command(&memory, list + 608, 0x10, 0x00080000u);
    command(&memory, list + 612, 0x01, 0x1000u);
    command(&memory, list + 616, 0x02, 0x1800u);
    command(&memory, list + 620, 0x12, vtype | (3u << 11));
    command(&memory, list + 624, 0x04, (3u << 16) | 3u);
    command(&memory, list + 628, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 608, 8));
    assert(psp_video_get_pixel(&video, 15, 15) != 0);

    /* PSP GE vertex color formats 5650, 5551, and 4444 decode correctly. */
    const uint32_t vertex_color_address = PSP_RAM_BASE + 0x1600;
    const uint32_t color_types[3] = {4u, 5u, 6u};
    const uint16_t packed_colors[3] = {0xF800u, 0xFC00u, 0xFF00u};
    for (uint32_t i = 0; i < 3; i++) {
        psp_ge_reset(&ge);
        psp_video_clear(&video, 0);
        put16(&memory, vertex_color_address, packed_colors[i]);
        put16(&memory, vertex_color_address + 2u, 16u);
        put16(&memory, vertex_color_address + 4u, 16u);
        uint32_t list_base = list + 640u + i * 32u;
        command(&memory, list_base, 0x10, 0x00080000u);
        command(&memory, list_base + 4u, 0x01, 0x1600u);
        command(&memory, list_base + 8u, 0x12,
                (color_types[i] << 2) | (2u << 7) | (1u << 23));
        command(&memory, list_base + 12u, 0x04, 1u);
        command(&memory, list_base + 16u, 0x0C, 0);
        assert(psp_ge_execute_list(&ge, list_base, 6));
        assert(psp_video_get_pixel(&video, 16, 16) == 0xFFFF0000u);
    }

    /* 3D vertices use world/view/projection and viewport state with depth testing. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    const uint32_t vertices_3d = PSP_RAM_BASE + 0x3000;
    const float depths[3] = {0.5f, -0.5f, 0.8f};
    const uint32_t colors_3d[3] = {
        0xFFFF0000u, 0xFF00FF00u, 0xFF0000FFu
    };
    for (uint32_t i = 0; i < 3; i++) {
        uint32_t address = vertices_3d + i * 16u;
        psp_memory_write32(&memory, address, colors_3d[i]);
        put_float(&memory, address + 4u, 0.0f);
        put_float(&memory, address + 8u, 0.0f);
        put_float(&memory, address + 12u, depths[i]);
    }
    command(&memory, list + 416, 0x10, 0x00080000u);
    command(&memory, list + 420, 0x3A, 3u);
    command(&memory, list + 424, 0x3B, encode_float24(0.5f));
    command(&memory, list + 428, 0x3C, 7u);
    command(&memory, list + 432, 0x3D, encode_float24(0.5f));
    command(&memory, list + 436, 0x3E, 0u);
    command(&memory, list + 440, 0x3F, encode_float24(2.0f));
    command(&memory, list + 444, 0x42, encode_float24(20.0f));
    command(&memory, list + 448, 0x43, encode_float24(-20.0f));
    command(&memory, list + 452, 0x44, encode_float24(100.0f));
    command(&memory, list + 456, 0x45, encode_float24(40.0f));
    command(&memory, list + 460, 0x46, encode_float24(40.0f));
    command(&memory, list + 464, 0x47, encode_float24(100.0f));
    command(&memory, list + 468, 0x01, 0x3000u);
    command(&memory, list + 472, 0x12, (7u << 2) | (3u << 7));
    command(&memory, list + 476, 0x23, 1u);
    command(&memory, list + 480, 0xDE, 4u);
    command(&memory, list + 484, 0x04, 3u);
    command(&memory, list + 488, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 416, 20));
    assert(psp_video_get_pixel(&video, 60, 30) == 0xFF00FF00u);

    /* Homogeneous right-plane clipping keeps the visible part of a
       triangle whose third vertex lies beyond the camera frustum. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    const float clip_positions[3][3] = {
        {-0.5f, -0.5f, 0.0f},
        { 2.0f, -0.5f, 0.0f},
        {-0.5f,  0.5f, 0.0f}
    };
    for (uint32_t i = 0; i < 3u; i++) {
        uint32_t address = vertices_3d + i * 16u;
        psp_memory_write32(&memory, address, 0xFFFF0000u);
        put_float(&memory, address + 4u, clip_positions[i][0]);
        put_float(&memory, address + 8u, clip_positions[i][1]);
        put_float(&memory, address + 12u, clip_positions[i][2]);
    }
    command(&memory, list + 1160u, 0x10, 0x00080000u);
    command(&memory, list + 1164u, 0x42, encode_float24(20.0f));
    command(&memory, list + 1168u, 0x43, encode_float24(-20.0f));
    command(&memory, list + 1172u, 0x45, encode_float24(40.0f));
    command(&memory, list + 1176u, 0x46, encode_float24(40.0f));
    command(&memory, list + 1180u, 0x01, 0x3000u);
    command(&memory, list + 1184u, 0x12, (7u << 2) | (3u << 7));
    command(&memory, list + 1188u, 0x04, (3u << 16) | 3u);
    command(&memory, list + 1192u, 0x0C, 0u);
    assert(psp_ge_execute_list(&ge, list + 1160u, 9u));
    assert(psp_video_get_pixel(&video, 55, 45) == 0xFFFF0000u);

    /* Exercise every homogeneous frustum plane: left, right, bottom,
       top, near, and far. Each clipped triangle must retain its center. */
    const float frustum_cases[6][3][3] = {
        {{-2.0f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}},
        {{-0.5f, -0.5f, 0.0f}, {2.0f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}},
        {{-0.5f, -2.0f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}},
        {{-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.0f, 2.0f, 0.0f}},
        {{-0.5f, -0.5f, -2.0f}, {0.5f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}},
        {{-0.5f, -0.5f, 2.0f}, {0.5f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}}
    };
    for (uint32_t plane = 0u; plane < 6u; plane++) {
        psp_ge_reset(&ge);
        psp_video_clear(&video, 0);
        for (uint32_t vertex = 0u; vertex < 3u; vertex++) {
            uint32_t address = vertices_3d + vertex * 16u;
            psp_memory_write32(&memory, address, 0xFF00FF00u);
            put_float(&memory, address + 4u, frustum_cases[plane][vertex][0]);
            put_float(&memory, address + 8u, frustum_cases[plane][vertex][1]);
            put_float(&memory, address + 12u, frustum_cases[plane][vertex][2]);
        }
        uint32_t list_base = list + 1200u + plane * 48u;
        command(&memory, list_base, 0x10, 0x00080000u);
        command(&memory, list_base + 4u, 0x42, encode_float24(20.0f));
        command(&memory, list_base + 8u, 0x43, encode_float24(-20.0f));
        command(&memory, list_base + 12u, 0x45, encode_float24(40.0f));
        command(&memory, list_base + 16u, 0x46, encode_float24(40.0f));
        command(&memory, list_base + 20u, 0x01, 0x3000u);
        command(&memory, list_base + 24u, 0x12, (7u << 2) | (3u << 7));
        command(&memory, list_base + 28u, 0x04, (3u << 16) | 3u);
        command(&memory, list_base + 32u, 0x0C, 0u);
        assert(psp_ge_execute_list(&ge, list_base, 9u));
        assert(psp_video_get_pixel(&video, 40, 40) == 0xFF00FF00u);
    }

    /* Directional diffuse lighting uses transformed vertex normals. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    const uint32_t lit_vertex = PSP_RAM_BASE + 0x3400u;
    psp_memory_write32(&memory, lit_vertex, 0xFFFFFFFFu);
    put_float(&memory, lit_vertex + 4u, 0.0f);
    put_float(&memory, lit_vertex + 8u, 0.0f);
    put_float(&memory, lit_vertex + 12u, 1.0f);
    put_float(&memory, lit_vertex + 16u, 0.0f);
    put_float(&memory, lit_vertex + 20u, 0.0f);
    put_float(&memory, lit_vertex + 24u, 0.0f);
    command(&memory, list + 800, 0x10, 0x00080000u);
    command(&memory, list + 804, 0x17, 1u);
    command(&memory, list + 808, 0x18, 1u);
    command(&memory, list + 812, 0x5F, 0u);
    command(&memory, list + 816, 0x6F, encode_float24(0.0f));
    command(&memory, list + 820, 0x70, encode_float24(0.0f));
    command(&memory, list + 824, 0x71, encode_float24(-1.0f));
    command(&memory, list + 828, 0x90, 0x00FF0000u);
    command(&memory, list + 832, 0x5C, 0u);
    command(&memory, list + 836, 0x42, encode_float24(20.0f));
    command(&memory, list + 840, 0x43, encode_float24(-20.0f));
    command(&memory, list + 844, 0x45, encode_float24(40.0f));
    command(&memory, list + 848, 0x46, encode_float24(40.0f));
    command(&memory, list + 852, 0x01, 0x3400u);
    command(&memory, list + 856, 0x12,
            (7u << 2) | (3u << 5) | (3u << 7));
    command(&memory, list + 860, 0x04, 1u);
    command(&memory, list + 864, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 800, 18));
    assert(psp_video_get_pixel(&video, 40, 40) == 0xFFFF0000u);

    /* A second directional light contributes independently to vertex color. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    command(&memory, list + 1600, 0x10, 0x00080000u);
    command(&memory, list + 1604, 0x17, 1u);
    command(&memory, list + 1608, 0x19, 1u);
    command(&memory, list + 1612, 0x60, 0u);
    command(&memory, list + 1616, 0x72, encode_float24(0.0f));
    command(&memory, list + 1620, 0x73, encode_float24(0.0f));
    command(&memory, list + 1624, 0x74, encode_float24(-1.0f));
    command(&memory, list + 1628, 0x93, 0x0000FF00u);
    command(&memory, list + 1632, 0x42, encode_float24(20.0f));
    command(&memory, list + 1636, 0x43, encode_float24(-20.0f));
    command(&memory, list + 1640, 0x45, encode_float24(40.0f));
    command(&memory, list + 1644, 0x46, encode_float24(40.0f));
    command(&memory, list + 1648, 0x01, 0x3400u);
    command(&memory, list + 1652, 0x12,
            (7u << 2) | (3u << 5) | (3u << 7));
    command(&memory, list + 1656, 0x04, 1u);
    command(&memory, list + 1660, 0x0C, 0u);
    assert(psp_ge_execute_list(&ge, list + 1600, 16u));
    assert(psp_video_get_pixel(&video, 40, 40) == 0xFF00FF00u);

    /* Third directional light is decoded and contributes to shading. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    command(&memory, list + 1680, 0x10, 0x00080000u);
    command(&memory, list + 1684, 0x17, 1u);
    command(&memory, list + 1688, 0x1A, 1u);
    command(&memory, list + 1692, 0x61, 0u);
    command(&memory, list + 1696, 0x75, encode_float24(0.0f));
    command(&memory, list + 1700, 0x76, encode_float24(0.0f));
    command(&memory, list + 1704, 0x77, encode_float24(-1.0f));
    command(&memory, list + 1708, 0x96, 0x000000FFu);
    command(&memory, list + 1712, 0x42, encode_float24(20.0f));
    command(&memory, list + 1716, 0x43, encode_float24(-20.0f));
    command(&memory, list + 1720, 0x45, encode_float24(40.0f));
    command(&memory, list + 1724, 0x46, encode_float24(40.0f));
    command(&memory, list + 1728, 0x01, 0x3400u);
    command(&memory, list + 1732, 0x12,
            (7u << 2) | (3u << 5) | (3u << 7));
    command(&memory, list + 1736, 0x04, 1u);
    command(&memory, list + 1740, 0x0C, 0u);
    assert(psp_ge_execute_list(&ge, list + 1680, 16u));
    assert(psp_video_get_pixel(&video, 40, 40) == 0xFF0000FFu);

    /* Fourth directional light is decoded and contributes to shading. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    command(&memory, list + 1760, 0x10, 0x00080000u);
    command(&memory, list + 1764, 0x17, 1u);
    command(&memory, list + 1768, 0x1B, 1u);
    command(&memory, list + 1772, 0x62, 0u);
    command(&memory, list + 1776, 0x78, encode_float24(0.0f));
    command(&memory, list + 1780, 0x79, encode_float24(0.0f));
    command(&memory, list + 1784, 0x7A, encode_float24(-1.0f));
    command(&memory, list + 1788, 0x99, 0x00FFFF00u);
    command(&memory, list + 1792, 0x42, encode_float24(20.0f));
    command(&memory, list + 1796, 0x43, encode_float24(-20.0f));
    command(&memory, list + 1800, 0x45, encode_float24(40.0f));
    command(&memory, list + 1804, 0x46, encode_float24(40.0f));
    command(&memory, list + 1808, 0x01, 0x3400u);
    command(&memory, list + 1812, 0x12,
            (7u << 2) | (3u << 5) | (3u << 7));
    command(&memory, list + 1816, 0x04, 1u);
    command(&memory, list + 1820, 0x0C, 0u);
    assert(psp_ge_execute_list(&ge, list + 1760, 16u));
    assert(psp_video_get_pixel(&video, 40, 40) == 0xFFFFFF00u);

    /* GE clear mode bypasses fragment blending and honors color/depth masks. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0xFF0000FFu);
    psp_memory_write32(&memory, PSP_VRAM_BASE +
        ((uint32_t)25u * PSP_VIDEO_STRIDE + 25u) * 4u, 0xFF0000FFu);
    psp_memory_write32(&memory, vertices, 0x80FF0000u);
    put16(&memory, vertices + 4u, 25u);
    put16(&memory, vertices + 6u, 25u);
    command(&memory, list + 880, 0x10, 0x00080000u);
    command(&memory, list + 884, 0x01, 0x1000u);
    command(&memory, list + 888, 0x12, vtype);
    command(&memory, list + 892, 0x21, 1u);
    command(&memory, list + 896, 0xDF, (3u << 8) | (2u << 4));
    command(&memory, list + 900, 0xD3, 1u);
    command(&memory, list + 904, 0x04, 1u);
    command(&memory, list + 908, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 880, 10));
    assert(psp_video_get_pixel(&video, 25, 25) == 0x80FF0000u);

    /* Clear-mode masks preserve color, alpha, and depth when requested. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0xFF0000FFu);
    psp_memory_write32(&memory, PSP_VRAM_BASE +
        ((uint32_t)25u * PSP_VIDEO_STRIDE + 25u) * 4u, 0xFF0000FFu);
    command(&memory, list + 912, 0x10, 0x00080000u);
    command(&memory, list + 916, 0x01, 0x1000u);
    command(&memory, list + 920, 0x12, vtype);
    command(&memory, list + 924, 0xD3, 1u | 0x700u);
    command(&memory, list + 928, 0x04, 1u);
    command(&memory, list + 932, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 912, 8));
    assert(psp_video_get_pixel(&video, 25, 25) == 0xFF0000FFu);
    assert(ge.depth_buffer[25u * PSP_VIDEO_WIDTH + 25u] == 0xFFFFu);

    /* PSP GE color test supports masked RGB equality and inequality. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0xFF0000FFu);
    psp_memory_write32(&memory, vertices, 0xFFFF0000u);
    put16(&memory, vertices + 4u, 26u);
    put16(&memory, vertices + 6u, 26u);
    command(&memory, list + 944, 0x10, 0x00080000u);
    command(&memory, list + 948, 0x01, 0x1000u);
    command(&memory, list + 952, 0x12, vtype);
    command(&memory, list + 956, 0xD8, 0u);
    command(&memory, list + 960, 0xD9, 0x0000FF00u);
    command(&memory, list + 964, 0xDA, 0x00FFFFFFu);
    command(&memory, list + 968, 0x27, 1u);
    command(&memory, list + 972, 0x04, 1u);
    command(&memory, list + 976, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 944, 10));
    assert(psp_video_get_pixel(&video, 26, 26) == 0xFF0000FFu);

    command(&memory, list + 980, 0xD9, 0x00FF0000u);
    command(&memory, list + 984, 0x04, 1u);
    command(&memory, list + 988, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 980, 4));
    assert(psp_video_get_pixel(&video, 26, 26) == 0xFFFF0000u);

    /* Indexed T8 and packed T4 textures use the loaded PSP CLUT. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    const uint32_t indexed_texture = PSP_RAM_BASE + 0x2500u;
    const uint32_t clut = PSP_RAM_BASE + 0x2600u;
    const uint32_t indexed_vertex = PSP_RAM_BASE + 0x2700u;
    psp_memory_write32(&memory, clut, 0xFFFF0000u);
    psp_memory_write32(&memory, clut + 4u, 0xFF00FF00u);
    psp_memory_write8(&memory, indexed_texture, 0u);
    psp_memory_write8(&memory, indexed_texture + 1u, 1u);
    put_float(&memory, indexed_vertex, 0.0f);
    put_float(&memory, indexed_vertex + 4u, 0.0f);
    psp_memory_write32(&memory, indexed_vertex + 8u, 0xFFFFFFFFu);
    put16(&memory, indexed_vertex + 12u, 50u);
    put16(&memory, indexed_vertex + 14u, 50u);
    command(&memory, list + 1000u, 0x10, 0x00080000u);
    command(&memory, list + 1004u, 0xB0, 0x2600u);
    command(&memory, list + 1008u, 0xC5, 3u | (0xFFu << 8));
    command(&memory, list + 1012u, 0xC4, 32u);
    command(&memory, list + 1016u, 0xA0, 0x2500u);
    command(&memory, list + 1020u, 0xA8, 0x00080002u);
    command(&memory, list + 1024u, 0xB8, 0x00000101u);
    command(&memory, list + 1028u, 0xC3, 5u);
    command(&memory, list + 1032u, 0x1E, 1u);
    command(&memory, list + 1036u, 0x01, 0x2700u);
    command(&memory, list + 1040u, 0x12, textured_vtype);
    command(&memory, list + 1044u, 0x04, 1u);
    command(&memory, list + 1048u, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 1000u, 16u));
    assert(psp_video_get_pixel(&video, 50, 50) == 0xFFFF0000u);

    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    psp_memory_write8(&memory, indexed_texture, 0x10u);
    psp_memory_write32(&memory, clut + 17u * 4u, 0xFF00FF00u);
    put_float(&memory, indexed_vertex, 1.0f);
    command(&memory, list + 1052u, 0x10, 0x00080000u);
    command(&memory, list + 1056u, 0xB0, 0x2600u);
    command(&memory, list + 1060u, 0xC5, 3u | (0xFFu << 8) | (1u << 16));
    command(&memory, list + 1064u, 0xC4, 32u);
    command(&memory, list + 1068u, 0xA0, 0x2500u);
    command(&memory, list + 1072u, 0xA8, 0x00080002u);
    command(&memory, list + 1076u, 0xB8, 0x00000101u);
    command(&memory, list + 1080u, 0xC3, 4u);
    command(&memory, list + 1084u, 0x1E, 1u);
    command(&memory, list + 1088u, 0x01, 0x2700u);
    command(&memory, list + 1092u, 0x12, textured_vtype);
    command(&memory, list + 1096u, 0x04, 1u);
    command(&memory, list + 1100u, 0x0C, 0);
    assert(psp_ge_execute_list(&ge, list + 1052u, 16u));
    assert(psp_video_get_pixel(&video, 50, 50) == 0xFF00FF00u);

    /* Triangles crossing the framebuffer edge are clipped, not dropped. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0);
    psp_memory_write32(&memory, vertices, 0xFFFF0000u);
    put16(&memory, vertices + 4u, (uint16_t)-10);
    put16(&memory, vertices + 6u, 10u);
    psp_memory_write32(&memory, vertices + 8u, 0xFFFF0000u);
    put16(&memory, vertices + 12u, 30u);
    put16(&memory, vertices + 14u, 10u);
    psp_memory_write32(&memory, vertices + 16u, 0xFFFF0000u);
    put16(&memory, vertices + 20u, 30u);
    put16(&memory, vertices + 22u, 50u);
    command(&memory, list + 1120u, 0x10, 0x00080000u);
    command(&memory, list + 1124u, 0x01, 0x1000u);
    command(&memory, list + 1128u, 0x12, vtype);
    command(&memory, list + 1132u, 0x04, (3u << 16) | 3u);
    command(&memory, list + 1136u, 0x0C, 0u);
    assert(psp_ge_execute_list(&ge, list + 1120u, 8u));
    assert(psp_video_get_pixel(&video, 0, 15) == 0xFFFF0000u);
    assert(psp_video_get_pixel(&video, 5, 45) == 0u);

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

    /* Fog is calculated from view-space Z and blended after texturing. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0u);
    const uint32_t fog_vertices = PSP_RAM_BASE + 0x3800u;
    const float fog_positions[3][3] = {
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.0f,  0.5f, -0.5f}
    };
    for (uint32_t i = 0u; i < 3u; i++) {
        uint32_t address = fog_vertices + i * 16u;
        psp_memory_write32(&memory, address, 0xFFFF0000u);
        put_float(&memory, address + 4u, fog_positions[i][0]);
        put_float(&memory, address + 8u, fog_positions[i][1]);
        put_float(&memory, address + 12u, fog_positions[i][2]);
    }
    command(&memory, list + 1840u, 0x10, 0x00080000u);
    command(&memory, list + 1844u, 0xCD, encode_float24(0.0f));
    command(&memory, list + 1848u, 0xCE, encode_float24(1.0f));
    command(&memory, list + 1852u, 0xCF, 0x000000FFu);
    command(&memory, list + 1856u, 0x1F, 1u);
    command(&memory, list + 1860u, 0x01, 0x3800u);
    command(&memory, list + 1864u, 0x12, (7u << 2) | (3u << 7));
    command(&memory, list + 1868u, 0x04, (3u << 16) | 3u);
    command(&memory, list + 1872u, 0x0C, 0u);
    assert(psp_ge_execute_list(&ge, list + 1840u, 9u));
    assert(psp_video_get_pixel(&video, 240, 150) == 0xFF0000FFu);

    /* Blinn-Phong specular highlights use material and per-light colors. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0u);
    const uint32_t specular_vertices = PSP_RAM_BASE + 0x3900u;
    const float specular_positions[3][3] = {
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.0f,  0.5f, -0.5f}
    };
    for (uint32_t i = 0u; i < 3u; i++) {
        uint32_t address = specular_vertices + i * 28u;
        psp_memory_write32(&memory, address, 0xFF000000u);
        put_float(&memory, address + 4u, 0.0f);
        put_float(&memory, address + 8u, 0.0f);
        put_float(&memory, address + 12u, 1.0f);
        put_float(&memory, address + 16u, specular_positions[i][0]);
        put_float(&memory, address + 20u, specular_positions[i][1]);
        put_float(&memory, address + 24u, specular_positions[i][2]);
    }
    command(&memory, list + 1880u, 0x10, 0x00080000u);
    command(&memory, list + 1884u, 0x17, 1u);
    command(&memory, list + 1888u, 0x18, 1u);
    command(&memory, list + 1892u, 0x5F, 0u);
    command(&memory, list + 1896u, 0x6F, encode_float24(0.0f));
    command(&memory, list + 1900u, 0x70, encode_float24(0.0f));
    command(&memory, list + 1904u, 0x71, encode_float24(-1.0f));
    command(&memory, list + 1908u, 0x90, 0u);
    command(&memory, list + 1912u, 0x91, 0x00FFFFFFu);
    command(&memory, list + 1916u, 0x55, 0x00FFFFFFu);
    command(&memory, list + 1920u, 0x58, encode_float24(1.0f));
    command(&memory, list + 1924u, 0x01, 0x3900u);
    command(&memory, list + 1928u, 0x12, (7u << 2) | (3u << 5) | (3u << 7));
    command(&memory, list + 1932u, 0x04, (3u << 16) | 3u);
    command(&memory, list + 1936u, 0x0Cu, 0u);
    assert(psp_ge_execute_list(&ge, list + 1880u, 15u));
    assert(ge.lighting_enabled);
    assert(ge.light0_enabled);
    assert(ge.material_specular == 0x00FFFFFFu);
    assert(ge.light_specular[0] == 0x00FFFFFFu);
    assert(ge.material_specular_coefficient == 1.0f);
    assert(ge.light0_direction[2] == -1.0f);
    assert(ge.primitives_drawn > 0u);
    uint32_t specular_pixel = psp_video_get_pixel(&video, 240, 136);
    assert((specular_pixel & 0xFF000000u) == 0xFF000000u);
    assert((specular_pixel & 0x00FFFFFFu) >= 0x00E0E0E0u);

    /* Diffuse-light tests need a white base material color. */
    for (uint32_t i = 0u; i < 3u; i++)
        psp_memory_write32(&memory, specular_vertices + i * 28u,
            0xFFFFFFFFu);

    /* Point lights use view-space position and distance attenuation. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0u);
    const uint32_t point_list = list + 1940u;
    const uint32_t point_commands[][2] = {
        {0x10u, 0x00080000u}, {0x17u, 1u}, {0x18u, 1u},
        {0x5Fu, 1u}, {0x63u, encode_float24(0.0f)},
        {0x64u, encode_float24(0.0f)}, {0x65u, encode_float24(0.0f)},
        {0x7Bu, encode_float24(1.0f)}, {0x7Cu, encode_float24(0.0f)},
        {0x7Du, encode_float24(0.0f)}, {0x90u, 0x00FFFFFFu},
        {0x91u, 0u}, {0x01u, 0x3900u},
        {0x12u, (7u << 2) | (3u << 5) | (3u << 7)},
        {0x04u, (3u << 16) | 3u}, {0x0Cu, 0u}
    };
    for (uint32_t i = 0u; i < sizeof(point_commands) / sizeof(point_commands[0]); i++)
        command(&memory, point_list + i * 4u,
            point_commands[i][0], point_commands[i][1]);
    assert(psp_ge_execute_list(&ge, point_list,
        (uint32_t)(sizeof(point_commands) / sizeof(point_commands[0]))));
    assert(ge.light0_type == 1u);
    assert(ge.light_position[0][2] == 0.0f);
    uint32_t point_pixel = psp_video_get_pixel(&video, 240, 136);
    assert((point_pixel & 0xFF000000u) == 0xFF000000u);
    assert((point_pixel & 0x00FFFFFFu) >= 0x00A0A0A0u);

    /* Spot lights additionally apply direction, cutoff and exponent. */
    psp_ge_reset(&ge);
    psp_video_clear(&video, 0u);
    const uint32_t spot_list = list + 2004u;
    const uint32_t spot_commands[][2] = {
        {0x10u, 0x00080000u}, {0x17u, 1u}, {0x18u, 1u},
        {0x5Fu, 2u}, {0x63u, encode_float24(0.0f)},
        {0x64u, encode_float24(0.0f)}, {0x65u, encode_float24(0.0f)},
        {0x6Fu, encode_float24(0.0f)}, {0x70u, encode_float24(0.0f)},
        {0x71u, encode_float24(-1.0f)},
        {0x7Bu, encode_float24(1.0f)}, {0x7Cu, encode_float24(0.0f)},
        {0x7Du, encode_float24(0.0f)}, {0x87u, encode_float24(2.0f)},
        {0x8Bu, encode_float24(0.5f)}, {0x90u, 0x00FFFFFFu},
        {0x91u, 0u}, {0x01u, 0x3900u},
        {0x12u, (7u << 2) | (3u << 5) | (3u << 7)},
        {0x04u, (3u << 16) | 3u}, {0x0Cu, 0u}
    };
    for (uint32_t i = 0u; i < sizeof(spot_commands) / sizeof(spot_commands[0]); i++)
        command(&memory, spot_list + i * 4u,
            spot_commands[i][0], spot_commands[i][1]);
    assert(psp_ge_execute_list(&ge, spot_list,
        (uint32_t)(sizeof(spot_commands) / sizeof(spot_commands[0]))));
    assert(ge.light0_type == 2u);
    assert(ge.light_spot_exponent[0] == 2.0f);
    assert(ge.light_spot_cutoff[0] == 0.5f);
    uint32_t spot_pixel = psp_video_get_pixel(&video, 240, 136);
    assert((spot_pixel & 0xFF000000u) == 0xFF000000u);
    /* Vertex lighting is evaluated before raster interpolation. The triangle
       spans the cone, so its center is intentionally dimmer than a point
       light while still receiving a visible spotlight contribution. */
    assert((spot_pixel & 0x00FFFFFFu) >= 0x00404040u);


    /* PSP GE block transfers copy 16-bit and 32-bit rectangular regions. */
    psp_ge_reset(&ge);
    const uint32_t transfer_src = PSP_RAM_BASE + 0x5000u;
    const uint32_t transfer_dst = PSP_RAM_BASE + 0x6000u;
    const uint32_t transfer_list = PSP_RAM_BASE + 0x7000u;
    for (uint32_t y = 0u; y < 3u; y++) {
        for (uint32_t x = 0u; x < 4u; x++) {
            psp_memory_write32(&memory,
                transfer_src + (y * 4u + x) * 4u,
                0xFF000000u | (y << 16) | (x << 8) | (x + y));
        }
    }
    const uint32_t transfer32[][2] = {
        {0xB2u, transfer_src & 0x00FFFFFFu},
        {0xB3u, ((transfer_src >> 8) & 0x000F0000u) | 4u},
        {0xB4u, transfer_dst & 0x00FFFFFFu},
        {0xB5u, ((transfer_dst >> 8) & 0x000F0000u) | 5u},
        {0xEBu, (1u << 10) | 1u},
        {0xECu, 2u},
        {0xEEu, (1u << 10) | 1u},
        {0xEAu, 1u},
        {0x0Cu, 0u}
    };
    for (uint32_t i = 0u; i < sizeof(transfer32) / sizeof(transfer32[0]); i++)
        command(&memory, transfer_list + i * 4u,
            (uint8_t)transfer32[i][0], transfer32[i][1]);
    assert(psp_ge_execute_list(&ge, transfer_list,
        (uint32_t)(sizeof(transfer32) / sizeof(transfer32[0]))));
    assert(psp_memory_read32(&memory, transfer_dst + (2u * 4u)) ==
        psp_memory_read32(&memory, transfer_src + (1u * 4u + 1u) * 4u));
    assert(psp_memory_read32(&memory, transfer_dst + (2u * 4u) + 4u) ==
        psp_memory_read32(&memory, transfer_src + (1u * 4u + 2u) * 4u));
    assert(psp_memory_read32(&memory, transfer_dst + (1u * 5u + 2u) * 4u) ==
        psp_memory_read32(&memory, transfer_src + (2u * 4u + 1u) * 4u));

    psp_ge_reset(&ge);
    const uint32_t transfer16_src = PSP_RAM_BASE + 0x8000u;
    const uint32_t transfer16_dst = PSP_RAM_BASE + 0x9000u;
    put16(&memory, transfer16_src, 0x1234u);
    put16(&memory, transfer16_src + 2u, 0xABCDu);
    const uint32_t transfer16[][2] = {
        {0xB2u, transfer16_src & 0x00FFFFFFu}, {0xB3u, ((transfer16_src >> 8) & 0x000F0000u) | 2u},
        {0xB4u, transfer16_dst & 0x00FFFFFFu}, {0xB5u, ((transfer16_dst >> 8) & 0x000F0000u) | 2u},
        {0xEEu, 1u}, {0xEAu, 0u}, {0x0Cu, 0u}
    };
    for (uint32_t i = 0u; i < sizeof(transfer16) / sizeof(transfer16[0]); i++)
        command(&memory, transfer_list + i * 4u,
            (uint8_t)transfer16[i][0], transfer16[i][1]);
    assert(psp_ge_execute_list(&ge, transfer_list,
        (uint32_t)(sizeof(transfer16) / sizeof(transfer16[0]))));
    assert(psp_memory_read8(&memory, transfer16_dst) == 0x34u);
    assert(psp_memory_read8(&memory, transfer16_dst + 1u) == 0x12u);
    assert(psp_memory_read8(&memory, transfer16_dst + 2u) == 0xCDu);
    assert(psp_memory_read8(&memory, transfer16_dst + 3u) == 0xABu);

    /* Invalid transfer dimensions/buffer widths fail without out-of-bounds access. */
    psp_ge_reset(&ge);
    const uint32_t invalid_transfer[][2] = {
        {0xB2u, transfer_src & 0x00FFFFFFu}, {0xB3u, ((transfer_src >> 8) & 0x000F0000u) | 1u},
        {0xB4u, transfer_dst & 0x00FFFFFFu}, {0xB5u, ((transfer_dst >> 8) & 0x000F0000u) | 1u},
        {0xEEu, 1u}, {0xEAu, 1u}, {0x0Cu, 0u}
    };
    for (uint32_t i = 0u; i < sizeof(invalid_transfer) / sizeof(invalid_transfer[0]); i++)
        command(&memory, transfer_list + i * 4u,
            (uint8_t)invalid_transfer[i][0], invalid_transfer[i][1]);
    assert(!psp_ge_execute_list(&ge, transfer_list,
        (uint32_t)(sizeof(invalid_transfer) / sizeof(invalid_transfer[0]))));

    /* Display presentation must use the display buffer, not GE draw target. */
    psp_ge_reset(&ge);
    const uint32_t draw_fb = PSP_VRAM_BASE + 0x4000u;
    const uint32_t screen_fb = PSP_VRAM_BASE + 0x8000u;
    ge.framebuffer_address = draw_fb;
    ge.framebuffer_stride = PSP_VIDEO_STRIDE;
    ge.framebuffer_format = 3u;
    ge.display_address = screen_fb;
    ge.display_stride = PSP_VIDEO_STRIDE;
    ge.display_format = 3u;
    psp_memory_write32(&memory, draw_fb, 0xFFFF0000u);
    psp_memory_write32(&memory, screen_fb, 0xFF00FF00u);
    psp_ge_present_framebuffer(&ge);
    assert(psp_video_get_pixel(&video, 0, 0) == 0xFF00FF00u);

    /* A null display address explicitly presents a black screen. */
    ge.display_address = 0u;
    psp_video_clear(&video, 0xFFFFFFFFu);
    psp_ge_present_framebuffer(&ge);
    assert(psp_video_get_pixel(&video, 0, 0) == 0xFF000000u);

    /* Framebuffer VRAM decoding covers all PSP pixel formats. */
    psp_ge_reset(&ge);
    const uint32_t display_fb = PSP_VRAM_BASE + 0x100u;
    ge.display_address = display_fb;
    ge.display_stride = PSP_VIDEO_STRIDE;
    ge.display_format = 3u;
    psp_memory_write32(&memory, display_fb + (7u * 4u),
                       0xFF123456u);
    psp_ge_present_framebuffer(&ge);
    assert(psp_video_get_pixel(&video, 7, 0) == 0xFF123456u);

    ge.display_format = 0u; /* 5650 */
    put16(&memory, display_fb + 2u, 0xF800u);
    psp_ge_present_framebuffer(&ge);
    assert(psp_video_get_pixel(&video, 1, 0) == 0xFFFF0000u);

    ge.display_format = 1u; /* 5551: opaque red */
    put16(&memory, display_fb + 4u, 0xF801u);
    psp_ge_present_framebuffer(&ge);
    assert(psp_video_get_pixel(&video, 2, 0) == 0xFFFF0000u);

    ge.display_format = 2u; /* 4444: opaque green */
    put16(&memory, display_fb + 6u, 0x0F0Fu);
    psp_ge_present_framebuffer(&ge);
    assert(psp_video_get_pixel(&video, 3, 0) == 0xFF00FF00u);

    /* An invalid full framebuffer range must not partially overwrite video. */
    psp_video_clear(&video, 0xFFABCDEFu);
    ge.display_address = PSP_VRAM_BASE + PSP_VRAM_SIZE - 16u;
    ge.display_format = 3u;
    psp_ge_present_framebuffer(&ge);
    assert(psp_video_get_pixel(&video, 0, 0) == 0xFFABCDEFu);

    /* GE raster output writes to the draw framebuffer, independently
       from the display framebuffer selected for presentation. */
    psp_ge_reset(&ge);
    const uint32_t ge_draw_target = PSP_VRAM_BASE + 0x10000u;
    const uint32_t ge_display_target = PSP_VRAM_BASE + 0xA0000u;
    const uint32_t ge_draw_list = PSP_RAM_BASE + 0xA100u;
    const uint32_t ge_draw_vertices = PSP_RAM_BASE + 0xA200u;
    psp_memory_write32(&memory, ge_draw_vertices, 0xFFFF0000u);
    put16(&memory, ge_draw_vertices + 4u, 10u);
    put16(&memory, ge_draw_vertices + 6u, 10u);
    psp_memory_write32(&memory, ge_draw_vertices + 8u, 0xFF00FF00u);
    put16(&memory, ge_draw_vertices + 12u, 40u);
    put16(&memory, ge_draw_vertices + 14u, 10u);
    psp_memory_write32(&memory, ge_draw_vertices + 16u, 0xFF0000FFu);
    put16(&memory, ge_draw_vertices + 20u, 10u);
    put16(&memory, ge_draw_vertices + 22u, 40u);
    command(&memory, ge_draw_list, 0x10u, 0x00080000u);
    command(&memory, ge_draw_list + 4u, 0x01u, 0xA200u);
    command(&memory, ge_draw_list + 8u, 0x12u, vtype);
    command(&memory, ge_draw_list + 12u, 0x04u, (3u << 16) | 3u);
    command(&memory, ge_draw_list + 16u, 0x0Cu, 0u);
    ge.framebuffer_address = ge_draw_target;
    ge.framebuffer_stride = PSP_VIDEO_STRIDE;
    ge.framebuffer_format = 3u;
    ge.display_address = ge_display_target;
    ge.display_stride = PSP_VIDEO_STRIDE;
    ge.display_format = 3u;
    psp_memory_write32(&memory, ge_draw_target +
        ((uint32_t)15u * PSP_VIDEO_STRIDE + 15u) * 4u, 0u);
    psp_memory_write32(&memory, ge_display_target +
        ((uint32_t)15u * PSP_VIDEO_STRIDE + 15u) * 4u, 0xFF123456u);
    assert(psp_ge_execute_list(&ge, ge_draw_list, 8u));
    assert(psp_memory_read32(&memory, ge_draw_target +
        ((uint32_t)15u * PSP_VIDEO_STRIDE + 15u) * 4u) != 0u);
    assert(psp_memory_read32(&memory, ge_display_target +
        ((uint32_t)15u * PSP_VIDEO_STRIDE + 15u) * 4u) == 0xFF123456u);

    /* Clear-mode color masking reads the prior draw-target pixel, not
       the unrelated host/display framebuffer contents. */
    psp_ge_reset(&ge);
    ge.framebuffer_address = ge_draw_target;
    ge.framebuffer_stride = PSP_VIDEO_STRIDE;
    ge.framebuffer_format = 3u;
    ge.display_address = ge_display_target;
    ge.display_stride = PSP_VIDEO_STRIDE;
    ge.display_format = 3u;
    const uint32_t draw_pixel_address = ge_draw_target +
        ((uint32_t)15u * PSP_VIDEO_STRIDE + 15u) * 4u;
    psp_memory_write32(&memory, draw_pixel_address, 0xFF0000FFu);
    psp_video_clear(&video, 0xFF000000u);
    ge.clear_mode = true;
    ge.clear_mode_mask = 0x100u; /* Preserve RGB; write alpha. */
    assert(psp_ge_execute_list(&ge, list, 8u));
    assert(psp_memory_read32(&memory, draw_pixel_address) == 0xFF0000FFu);

    psp_ge_destroy(&ge);
    psp_video_destroy(&video);
    psp_memory_destroy(&memory);
    puts("PSP GE tests passed");
    return 0;
}
