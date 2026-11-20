#include "psp_memory.h"

#include <stdlib.h>
#include <string.h>

static bool translate_address(
    PSP_Memory *memory,
    uint32_t address,
    uint32_t *offset
)
{
    if (memory == NULL || memory->ram == NULL ||
        offset == NULL)
        return false;

    if (address >= PSP_RAM_BASE)
    {
        uint32_t relative = address - PSP_RAM_BASE;

        if (relative >= memory->size)
            return false;

        *offset = relative;
        return true;
    }

    if (address >= memory->size)
        return false;

    *offset = address;
    return true;
}

bool psp_memory_init(PSP_Memory *memory)
{
    if (memory == NULL)
        return false;

    /* Leave a predictable empty state if allocation fails. */
    memory->ram = NULL;
    memory->size = 0;

    memory->ram = calloc(1, PSP_RAM_SIZE);

    if (memory->ram == NULL)
        return false;

    memory->size = PSP_RAM_SIZE;

    return true;
}

void psp_memory_destroy(PSP_Memory *memory)
{
    if (memory == NULL)
        return;

    free(memory->ram);

    memory->ram = NULL;
    memory->size = 0;
}

uint8_t psp_memory_read8(
    PSP_Memory *memory,
    uint32_t address
)
{
    uint32_t offset;

    if (!translate_address(memory, address, &offset))
        return 0;

    return memory->ram[offset];
}

uint32_t psp_memory_read32(
    PSP_Memory *memory,
    uint32_t address
)
{
    uint32_t offset;

    if (!translate_address(memory, address, &offset))
        return 0;

    if (memory->size - offset < 4)
        return 0;

    return (uint32_t)memory->ram[offset]
         | ((uint32_t)memory->ram[offset + 1] << 8)
         | ((uint32_t)memory->ram[offset + 2] << 16)
         | ((uint32_t)memory->ram[offset + 3] << 24);
}

void psp_memory_write8(
    PSP_Memory *memory,
    uint32_t address,
    uint8_t value
)
{
    uint32_t offset;

    if (!translate_address(memory, address, &offset))
        return;

    memory->ram[offset] = value;
}

void psp_memory_write32(
    PSP_Memory *memory,
    uint32_t address,
    uint32_t value
)
{
    uint32_t offset;

    if (!translate_address(memory, address, &offset))
        return;

    if (memory->size - offset < 4)
        return;

    memory->ram[offset] = (uint8_t)value;
    memory->ram[offset + 1] = (uint8_t)(value >> 8);
    memory->ram[offset + 2] = (uint8_t)(value >> 16);
    memory->ram[offset + 3] = (uint8_t)(value >> 24);
}
