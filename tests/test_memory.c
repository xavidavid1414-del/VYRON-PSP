#include "psp_memory.h"

#include <assert.h>
#include <stdint.h>
#include <stddef.h>

int main(void)
{
    uint8_t ram[16] = {0};
    static uint8_t vram[PSP_VRAM_SIZE];
    PSP_Memory memory = {
        .ram = ram,
        .size = sizeof(ram),
        .vram = vram,
        .vram_size = sizeof(vram)
    };

    assert(psp_memory_is_valid(&memory, PSP_RAM_BASE, 16));
    assert(psp_memory_is_valid(&memory, PSP_RAM_BASE + 15u, 1));
    assert(!psp_memory_is_valid(&memory, PSP_RAM_BASE + 15u, 2));
    assert(!psp_memory_is_valid(&memory, PSP_RAM_BASE + 16u, 1));
    assert(!psp_memory_is_valid(&memory, PSP_RAM_BASE, 0));
    assert(!psp_memory_is_valid(&memory, 0xFFFFFFFFu, 2));
    assert(!psp_memory_is_valid(NULL, PSP_RAM_BASE, 1));

    /* PSP EDRAM is a separate 2 MiB address region. */
    assert(psp_memory_is_valid(&memory, PSP_VRAM_BASE, PSP_VRAM_SIZE));
    assert(psp_memory_is_valid(&memory, PSP_VRAM_BASE + PSP_VRAM_SIZE - 1u, 1));
    assert(!psp_memory_is_valid(&memory, PSP_VRAM_BASE + PSP_VRAM_SIZE, 1));
    assert(!psp_memory_is_valid(&memory, PSP_VRAM_BASE + PSP_VRAM_SIZE - 1u, 2));
    psp_memory_write32(&memory, PSP_VRAM_BASE + 0x100u, 0xA1B2C3D4u);
    assert(psp_memory_read32(&memory, PSP_VRAM_BASE + 0x100u) == 0xA1B2C3D4u);
    psp_memory_write8(&memory, PSP_VRAM_BASE + PSP_VRAM_SIZE - 1u, 0x7Eu);
    assert(psp_memory_read8(&memory, PSP_VRAM_BASE + PSP_VRAM_SIZE - 1u) == 0x7Eu);

    psp_memory_write32(&memory, PSP_RAM_BASE, 0x12345678u);
    assert(psp_memory_read32(&memory, PSP_RAM_BASE) == 0x12345678u);

    psp_memory_write8(&memory, PSP_RAM_BASE + 15u, 0xABu);
    assert(psp_memory_read8(&memory, PSP_RAM_BASE + 15u) == 0xABu);

    /* Invalid writes must not modify the final byte. */
    psp_memory_write32(&memory, PSP_RAM_BASE + 15u, 0xFFFFFFFFu);
    assert(psp_memory_read8(&memory, PSP_RAM_BASE + 15u) == 0xABu);

    return 0;
}
