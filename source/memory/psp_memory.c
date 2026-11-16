#include "psp_memory.h"
#include <stdlib.h>
#include <string.h>

bool psp_memory_init(PSP_Memory *memory)
{
    if (memory == NULL)
        return false;

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

uint8_t psp_memory_read8(PSP_Memory *memory, uint32_t address)
{
    if (memory == NULL || memory->ram == NULL ||
        address >= memory->size)
        return 0;

    return memory->ram[address];
}

uint32_t psp_memory_read32(PSP_Memory *memory, uint32_t address)
{
    if (memory == NULL || memory->ram == NULL ||
        address > memory->size ||
        memory->size - address < 4)
        return 0;

    return (uint32_t)memory->ram[address] |
           ((uint32_t)memory->ram[address + 1] << 8) |
           ((uint32_t)memory->ram[address + 2] << 16) |
           ((uint32_t)memory->ram[address + 3] << 24);
}

void psp_memory_write8(PSP_Memory *memory, uint32_t address, uint8_t value)
{
    if (memory == NULL || memory->ram == NULL ||
        address >= memory->size)
        return;

    memory->ram[address] = value;
}

void psp_memory_write32(PSP_Memory *memory, uint32_t address, uint32_t value)
{
    if (memory == NULL || memory->ram == NULL ||
        address > memory->size ||
        memory->size - address < 4)
        return;

    memory->ram[address] = (uint8_t)value;
    memory->ram[address + 1] = (uint8_t)(value >> 8);
    memory->ram[address + 2] = (uint8_t)(value >> 16);
    memory->ram[address + 3] = (uint8_t)(value >> 24);
}
