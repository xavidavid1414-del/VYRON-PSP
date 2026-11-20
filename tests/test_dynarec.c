#include "cpu/psp_cpu.h"
#include "cpu/psp_dynarec.h"
#include "cpu/psp_interpreter.h"
#include "memory/psp_memory.h"

#include <assert.h>
#include <stdint.h>

#define CODE (PSP_RAM_BASE + 0x100u)

static uint32_t addiu(uint32_t rt, uint32_t rs, uint16_t imm)
{
    return (0x09u << 26) | (rs << 21) | (rt << 16) | imm;
}

int main(void)
{
    PSP_Memory memory = {0};
    PSP_CPU translated, reference;
    PSP_Dynarec dynarec;

    assert(psp_memory_init(&memory));
    psp_cpu_init(&translated);
    psp_cpu_init(&reference);
    psp_dynarec_init(&dynarec);

    /* Two ALU ops, a taken branch, and its delay slot. */
    psp_memory_write32(&memory, CODE + 0u, addiu(1, 0, 1));
    psp_memory_write32(&memory, CODE + 4u, addiu(1, 1, 2));
    psp_memory_write32(&memory, CODE + 8u,
                       (0x04u << 26) | (1u << 21) | (1u << 16) | 1u);
    psp_memory_write32(&memory, CODE + 12u, addiu(2, 0, 7));
    psp_memory_write32(&memory, CODE + 16u, addiu(3, 0, 9));
    psp_memory_write32(&memory, CODE + 20u, addiu(4, 0, 11));

    translated.pc = CODE;
    reference.pc = CODE;
    assert(psp_dynarec_run(&dynarec, &translated, &memory, 4));
    for (unsigned i = 0; i < 4; ++i)
        assert(psp_interpreter_step(&reference, &memory));

    assert(translated.pc == reference.pc);
    assert(translated.pc == CODE + 16u);
    assert(translated.gpr[1] == reference.gpr[1]);
    assert(translated.gpr[2] == reference.gpr[2]);
    assert(translated.gpr[2] == 7u);
    assert(translated.gpr[3] == 0u);

    assert(psp_dynarec_run(&dynarec, &translated, &memory, 2));
    assert(psp_interpreter_step(&reference, &memory));
    assert(psp_interpreter_step(&reference, &memory));
    assert(translated.pc == reference.pc);
    assert(translated.gpr[3] == 9u);
    assert(translated.gpr[4] == 11u);

    /* A RAM write to translated code must invalidate its cached block. */
    psp_dynarec_reset(&dynarec);
    psp_cpu_reset(&translated);
    translated.pc = CODE + 0x100u;
    psp_memory_write32(&memory, translated.pc, addiu(5, 0, 1));
    assert(psp_dynarec_run(&dynarec, &translated, &memory, 1));
    assert(translated.gpr[5] == 1u);
    uint64_t invalidations_before = dynarec.cache_invalidations;

    translated.pc = CODE + 0x100u;
    psp_memory_write32(&memory, translated.pc, addiu(5, 0, 2));
    assert(psp_dynarec_run(&dynarec, &translated, &memory, 1));
    assert(translated.gpr[5] == 2u);
    assert(dynarec.cache_invalidations > invalidations_before);
    assert(dynarec.translated_blocks >= 2u);
    assert(dynarec.executed_instructions == 2u);

    psp_memory_destroy(&memory);
    return 0;
}
