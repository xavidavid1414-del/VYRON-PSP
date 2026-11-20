#include "psp_dynarec.h"
#include "psp_interpreter.h"

#include <string.h>

void psp_dynarec_init(PSP_Dynarec *dynarec)
{
    if (dynarec) memset(dynarec, 0, sizeof(*dynarec));
}

void psp_dynarec_reset(PSP_Dynarec *dynarec)
{
    psp_dynarec_init(dynarec);
}

static PSP_DynarecBlock *find_block(PSP_Dynarec *dynarec,
                                    uint32_t pc, uint32_t generation)
{
    for (uint32_t i = 0; i < PSP_DYNAREC_CACHE_BLOCKS; ++i) {
        PSP_DynarecBlock *block = &dynarec->blocks[i];
        if (!block->valid || block->start_pc != pc) continue;
        if (block->page_generation == generation) {
            dynarec->cache_hits++;
            return block;
        }
        block->valid = false;
        dynarec->cache_invalidations++;
    }
    return NULL;
}

static PSP_DynarecBlock *translate_block(PSP_Dynarec *dynarec,
                                         PSP_Memory *memory, uint32_t pc,
                                         uint32_t generation)
{
    PSP_DynarecBlock *block =
        &dynarec->blocks[dynarec->next_victim];
    dynarec->next_victim =
        (dynarec->next_victim + 1u) % PSP_DYNAREC_CACHE_BLOCKS;
    memset(block, 0, sizeof(*block));
    block->start_pc = pc;
    block->page_generation = generation;

    uint32_t page_offset = (pc - PSP_RAM_BASE) % PSP_MEMORY_PAGE_SIZE;
    uint32_t bytes_left = PSP_MEMORY_PAGE_SIZE - page_offset;
    uint32_t max_count = bytes_left / 4u;
    if (max_count > PSP_DYNAREC_BLOCK_INSTRUCTIONS)
        max_count = PSP_DYNAREC_BLOCK_INSTRUCTIONS;

    while (block->instruction_count < max_count) {
        uint32_t index = block->instruction_count;
        uint32_t address = pc + index * 4u;
        if (!psp_memory_is_valid(memory, address, 4)) break;
        block->instructions[index] = psp_memory_read32(memory, address);
        block->instruction_count++;
    }

    if (block->instruction_count == 0) {
        block->valid = false;
        return NULL;
    }
    block->valid = true;
    dynarec->translated_blocks++;
    return block;
}

bool psp_dynarec_run(PSP_Dynarec *dynarec, PSP_CPU *cpu,
                     PSP_Memory *memory, uint32_t instruction_budget)
{
    if (!dynarec || !cpu || !memory || cpu->halted) return false;
    if (instruction_budget == 0) return true;

    uint32_t executed = 0;
    while (executed < instruction_budget) {
        uint32_t pc = cpu->pc;
        if ((pc & 3u) != 0 || !psp_memory_is_valid(memory, pc, 4)) {
            if (!psp_interpreter_step(cpu, memory)) return false;
            executed++;
            dynarec->executed_instructions++;
            continue;
        }

        uint32_t generation = psp_memory_page_generation(memory, pc);
        if (generation == 0) {
            if (!psp_interpreter_step(cpu, memory)) return false;
            executed++;
            dynarec->executed_instructions++;
            continue;
        }

        PSP_DynarecBlock *block = find_block(dynarec, pc, generation);
        if (!block)
            block = translate_block(dynarec, memory, pc, generation);
        if (!block) {
            if (!psp_interpreter_step(cpu, memory)) return false;
            executed++;
            dynarec->executed_instructions++;
            continue;
        }

        for (uint32_t i = 0;
             i < block->instruction_count && executed < instruction_budget;
             ++i) {
            uint32_t current_pc = cpu->pc;
            uint32_t expected_pc = block->start_pc + i * 4u;
            if (current_pc != expected_pc) break;

            bool was_delay_slot = cpu->branch_pending;
            uint32_t instruction = block->instructions[i];
            if (!psp_interpreter_execute_instruction(cpu, memory, instruction))
                return false;

            executed++;
            dynarec->executed_instructions++;

            /* A branch target is committed after its delay-slot instruction. */
            if (was_delay_slot || cpu->pc != current_pc + 4u)
                break;
        }
    }
    return true;
}
