#ifndef PSP_MEMORY_BLOCKS_H
#define PSP_MEMORY_BLOCKS_H
#include <stdbool.h>
#include <stdint.h>
#include "psp_memory.h"
#define PSP_MEMORY_BLOCK_MAX 64u
#define PSP_MEMORY_PARTITION_USER 2u
#define PSP_MEMORY_BLOCK_LOW 0u
#define PSP_MEMORY_BLOCK_HIGH 1u
#define PSP_MEMORY_BLOCK_ADDR 2u
typedef struct { uint32_t uid, address, size; bool active; } PSP_MemoryBlock;
typedef struct { PSP_Memory *memory; PSP_MemoryBlock blocks[PSP_MEMORY_BLOCK_MAX]; uint32_t arena_start, arena_end, next_uid; } PSP_MemoryBlocks;
void psp_memory_blocks_init(PSP_MemoryBlocks *manager, PSP_Memory *memory);
void psp_memory_blocks_reset(PSP_MemoryBlocks *manager);
uint32_t psp_memory_block_alloc(PSP_MemoryBlocks *manager, uint32_t partition, uint32_t type, uint32_t size, uint32_t address);
bool psp_memory_block_free(PSP_MemoryBlocks *manager, uint32_t uid);
uint32_t psp_memory_block_head(PSP_MemoryBlocks *manager, uint32_t uid);
#endif
