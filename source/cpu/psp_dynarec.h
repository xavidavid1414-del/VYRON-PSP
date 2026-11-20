#ifndef PSP_DYNAREC_H
#define PSP_DYNAREC_H

#include <stdbool.h>
#include <stdint.h>

#include "psp_cpu.h"
#include "../memory/psp_memory.h"

#define PSP_DYNAREC_CACHE_BLOCKS 64u
#define PSP_DYNAREC_BLOCK_INSTRUCTIONS 32u

typedef struct {
    uint32_t start_pc;
    uint32_t page_generation;
    uint8_t instruction_count;
    bool valid;
    uint32_t instructions[PSP_DYNAREC_BLOCK_INSTRUCTIONS];
} PSP_DynarecBlock;

typedef struct {
    PSP_DynarecBlock blocks[PSP_DYNAREC_CACHE_BLOCKS];
    uint32_t next_victim;
    uint64_t translated_blocks;
    uint64_t cache_hits;
    uint64_t cache_invalidations;
    uint64_t executed_instructions;
} PSP_Dynarec;

void psp_dynarec_init(PSP_Dynarec *dynarec);
void psp_dynarec_reset(PSP_Dynarec *dynarec);
bool psp_dynarec_run(PSP_Dynarec *dynarec, PSP_CPU *cpu,
                     PSP_Memory *memory, uint32_t instruction_budget);

#endif
