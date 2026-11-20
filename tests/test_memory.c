#include "psp_memory.h"

#include <assert.h>
#include <stdint.h>
#include <stddef.h>

int main(void)
{
    uint8_t ram[16] = {0};
    PSP_Memory memory = {
        .ram = ram,
        .size = sizeof(ram)
    };

    assert(psp_memory_is_valid(&memory, PSP_RAM_BASE, 16));
    assert(psp_memory_is_valid(&memory, PSP_RAM_BASE + 15u, 1));
    assert(!psp_memory_is_valid(&memory, PSP_RAM_BASE + 15u, 2));
    assert(!psp_memory_is_valid(&memory, PSP_RAM_BASE + 16u, 1));
    assert(!psp_memory_is_valid(&memory, PSP_RAM_BASE, 0));
    assert(!psp_memory_is_valid(&memory, 0xFFFFFFFFu, 2));
    assert(!psp_memory_is_valid(NULL, PSP_RAM_BASE, 1));

    psp_memory_write32(&memory, PSP_RAM_BASE, 0x12345678u);
    assert(psp_memory_read32(&memory, PSP_RAM_BASE) == 0x12345678u);

    psp_memory_write8(&memory, PSP_RAM_BASE + 15u, 0xABu);
    assert(psp_memory_read8(&memory, PSP_RAM_BASE + 15u) == 0xABu);

    /* Invalid writes must not modify the final byte. */
    psp_memory_write32(&memory, PSP_RAM_BASE + 15u, 0xFFFFFFFFu);
    assert(psp_memory_read8(&memory, PSP_RAM_BASE + 15u) == 0xABu);

    return 0;
}
