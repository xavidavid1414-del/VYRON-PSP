#include "psp_memory.h"

#include <stdlib.h>
#include <string.h>

static void mark_ram_write(PSP_Memory *memory, uint32_t offset, uint32_t length)
{
    if (!memory || !memory->page_generation || length == 0) return;
    uint32_t first = offset / PSP_MEMORY_PAGE_SIZE;
    uint32_t last = (offset + length - 1u) / PSP_MEMORY_PAGE_SIZE;
    if (last >= memory->page_count) return;
    for (uint32_t page = first; page <= last; ++page) {
        memory->page_generation[page]++;
        if (memory->page_generation[page] == 0)
            memory->page_generation[page] = 1;
    }
}

static bool translate_vram_address(PSP_Memory *memory, uint32_t address,
                                  uint32_t *offset)
{
    if (memory == NULL || memory->vram == NULL || offset == NULL ||
        address < PSP_VRAM_BASE)
        return false;
    uint32_t relative = address - PSP_VRAM_BASE;
    if (relative >= memory->vram_size)
        return false;
    *offset = relative;
    return true;
}

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

uint32_t psp_memory_page_generation(PSP_Memory *memory, uint32_t address)
{
    uint32_t offset;
    if (!memory || !memory->page_generation ||
        !translate_address(memory, address, &offset))
        return 0;
    return memory->page_generation[offset / PSP_MEMORY_PAGE_SIZE];
}

bool psp_memory_is_valid(
    PSP_Memory *memory,
    uint32_t address,
    uint32_t length
)
{
    uint32_t offset;

    if (length == 0)
        return false;
    if (translate_vram_address(memory, address, &offset))
        return length <= memory->vram_size - offset;
    if (!translate_address(memory, address, &offset))
        return false;
    return length <= memory->size - offset;
}

bool psp_memory_init(PSP_Memory *memory)
{
    if (memory == NULL)
        return false;

    /* Leave a predictable empty state if allocation fails. */
    memory->ram = NULL;
    memory->size = 0;
    memory->vram = NULL;
    memory->vram_size = 0;
    memory->page_generation = NULL;
    memory->page_count = 0;

    memory->ram = calloc(1, PSP_RAM_SIZE);
    if (memory->ram == NULL)
        return false;
    memory->vram = calloc(1, PSP_VRAM_SIZE);
    if (memory->vram == NULL) {
        free(memory->ram);
        memory->ram = NULL;
        return false;
    }
    memory->size = PSP_RAM_SIZE;
    memory->vram_size = PSP_VRAM_SIZE;
    memory->page_count = (memory->size + PSP_MEMORY_PAGE_SIZE - 1u) /
                         PSP_MEMORY_PAGE_SIZE;
    memory->page_generation = calloc(memory->page_count, sizeof(uint32_t));
    if (!memory->page_generation) {
        free(memory->vram);
        free(memory->ram);
        memory->ram = NULL;
        memory->vram = NULL;
        memory->size = 0;
        memory->vram_size = 0;
        memory->page_count = 0;
        return false;
    }
    for (uint32_t i = 0; i < memory->page_count; ++i)
        memory->page_generation[i] = 1u;
    return true;
}

void psp_memory_destroy(PSP_Memory *memory)
{
    if (memory == NULL)
        return;

    free(memory->ram);
    free(memory->vram);
    free(memory->page_generation);
    memory->page_generation = NULL;
    memory->page_count = 0;
    memory->ram = NULL;
    memory->size = 0;
    memory->vram = NULL;
    memory->vram_size = 0;
}

uint8_t psp_memory_read8(
    PSP_Memory *memory,
    uint32_t address
)
{
    uint32_t offset;

    if (translate_vram_address(memory, address, &offset))
        return memory->vram[offset];
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

    uint8_t *bytes;
    if (translate_vram_address(memory, address, &offset)) {
        if (memory->vram_size - offset < 4) return 0;
        bytes = memory->vram + offset;
    } else {
        if (!translate_address(memory, address, &offset) ||
            memory->size - offset < 4) return 0;
        bytes = memory->ram + offset;
    }
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

void psp_memory_write8(
    PSP_Memory *memory,
    uint32_t address,
    uint8_t value
)
{
    uint32_t offset;

    if (translate_vram_address(memory, address, &offset)) {
        memory->vram[offset] = value;
        return;
    }
    if (!translate_address(memory, address, &offset))
        return;
    memory->ram[offset] = value;
    mark_ram_write(memory, offset, 1u);
}

void psp_memory_write32(
    PSP_Memory *memory,
    uint32_t address,
    uint32_t value
)
{
    uint32_t offset;

    uint8_t *bytes;
    if (translate_vram_address(memory, address, &offset)) {
        if (memory->vram_size - offset < 4) return;
        bytes = memory->vram + offset;
    } else {
        if (!translate_address(memory, address, &offset) ||
            memory->size - offset < 4) return;
        bytes = memory->ram + offset;
    }
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
    uint32_t vram_offset;
    if (!translate_vram_address(memory, address, &vram_offset))
        mark_ram_write(memory, offset, 4u);
}
