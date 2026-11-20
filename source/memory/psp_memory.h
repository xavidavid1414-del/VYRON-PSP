#ifndef PSP_MEMORY_H
#define PSP_MEMORY_H

#include <stdint.h>
#include <stdbool.h>

#define PSP_RAM_SIZE (32 * 1024 * 1024)
#define PSP_RAM_BASE 0x08000000u
#define PSP_VRAM_SIZE (2 * 1024 * 1024)
#define PSP_VRAM_BASE 0x04000000u

#define PSP_MEMORY_PAGE_SIZE 4096u

typedef struct {
    uint8_t *ram;
    uint32_t size;
    uint8_t *vram;
    uint32_t vram_size;
    uint32_t *page_generation;
    uint32_t page_count;
} PSP_Memory;

bool psp_memory_is_valid(PSP_Memory *memory, uint32_t address, uint32_t length);
bool psp_memory_init(PSP_Memory *memory);
uint32_t psp_memory_page_generation(PSP_Memory *memory, uint32_t address);
void psp_memory_destroy(PSP_Memory *memory);

uint8_t psp_memory_read8(PSP_Memory *memory, uint32_t address);
uint32_t psp_memory_read32(PSP_Memory *memory, uint32_t address);

void psp_memory_write8(PSP_Memory *memory, uint32_t address, uint8_t value);
void psp_memory_write32(PSP_Memory *memory, uint32_t address, uint32_t value);

#endif
