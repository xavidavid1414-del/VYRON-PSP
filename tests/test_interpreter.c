#include "psp_interpreter.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define COP0_BADVADDR 8u
#define COP0_CAUSE 13u
#define EXCEPTION_VECTOR (PSP_RAM_BASE + 0x180u)

int main(void)
{
    uint8_t ram[16] = {0};
    PSP_Memory memory = { .ram = ram, .size = sizeof(ram) };
    PSP_CPU cpu;

    /* LW r2, 0(r1): the four-byte read begins at the RAM boundary. */
    const uint32_t lw_r2_r1 = (0x23u << 26) | (1u << 21) | (2u << 16);
    memcpy(ram, &lw_r2_r1, sizeof(lw_r2_r1));

    psp_cpu_init(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = PSP_RAM_BASE + 16u;
    cpu.gpr[2] = 0xA5A5A5A5u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (4u << 2));
    assert(cpu.cop0[COP0_BADVADDR] == PSP_RAM_BASE + 16u);
    assert(cpu.gpr[2] == 0xA5A5A5A5u);

    /* An unaligned LW must raise ADEL before reading memory. */
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = PSP_RAM_BASE + 1u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (4u << 2));
    assert(cpu.cop0[COP0_BADVADDR] == PSP_RAM_BASE + 1u);

    /* SW r2, 0(r1): an invalid four-byte write must raise ADES. */
    const uint32_t sw_r2_r1 = (0x2Bu << 26) | (1u << 21) | (2u << 16);
    memcpy(ram, &sw_r2_r1, sizeof(sw_r2_r1));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = PSP_RAM_BASE + 16u;
    cpu.gpr[2] = 0x12345678u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (5u << 2));
    assert(cpu.cop0[COP0_BADVADDR] == PSP_RAM_BASE + 16u);

    /* LH with an unaligned address must raise ADEL. */
    const uint32_t lh_r2_r1 = (0x21u << 26) | (1u << 21) | (2u << 16);
    memcpy(ram, &lh_r2_r1, sizeof(lh_r2_r1));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = PSP_RAM_BASE + 1u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (4u << 2));
    assert(cpu.cop0[COP0_BADVADDR] == PSP_RAM_BASE + 1u);

    /* SH with an unaligned address must raise ADES. */
    const uint32_t sh_r2_r1 = (0x29u << 26) | (1u << 21) | (2u << 16);
    memcpy(ram, &sh_r2_r1, sizeof(sh_r2_r1));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = PSP_RAM_BASE + 1u;
    cpu.gpr[2] = 0x12345678u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (5u << 2));
    assert(cpu.cop0[COP0_BADVADDR] == PSP_RAM_BASE + 1u);

    /* LB beyond RAM must raise ADEL. */
    const uint32_t lb_r2_r1 = (0x20u << 26) | (1u << 21) | (2u << 16);
    memcpy(ram, &lb_r2_r1, sizeof(lb_r2_r1));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = PSP_RAM_BASE + 16u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (4u << 2));
    assert(cpu.cop0[COP0_BADVADDR] == PSP_RAM_BASE + 16u);

    /* SB beyond RAM must raise ADES. */
    const uint32_t sb_r2_r1 = (0x28u << 26) | (1u << 21) | (2u << 16);
    memcpy(ram, &sb_r2_r1, sizeof(sb_r2_r1));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = PSP_RAM_BASE + 16u;
    cpu.gpr[2] = 0x5Au;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (5u << 2));
    assert(cpu.cop0[COP0_BADVADDR] == PSP_RAM_BASE + 16u);

    /* Fetching an instruction beyond RAM must raise ADEL as well. */
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE + 16u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (4u << 2));
    assert(cpu.cop0[COP0_BADVADDR] == PSP_RAM_BASE + 16u);


    /* ADDI signed overflow must raise OV without changing rt. */
    const uint32_t addi_overflow =
        (0x08u << 26) | (1u << 21) | (2u << 16) | 1u;
    memcpy(ram, &addi_overflow, sizeof(addi_overflow));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 0x7FFFFFFFu;
    cpu.gpr[2] = 0xCAFEBABEu;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (12u << 2));
    assert(cpu.cop0[14] == PSP_RAM_BASE);
    assert(cpu.gpr[2] == 0xCAFEBABEu);

    /* A reserved SPECIAL function must raise RI. */
    const uint32_t reserved_special = 0x0000003Fu;
    memcpy(ram, &reserved_special, sizeof(reserved_special));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (10u << 2));
    assert(cpu.cop0[14] == PSP_RAM_BASE);


    /* BEQ executes its delay-slot instruction before jumping. */
    const uint32_t beq_r1_r1 =
        (0x04u << 26) | (1u << 21) | (1u << 16) | 2u;
    const uint32_t nop = 0u;
    memcpy(ram, &beq_r1_r1, sizeof(beq_r1_r1));
    memcpy(ram + 4, &nop, sizeof(nop));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 7u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 4u);
    assert(cpu.branch_pending);

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 12u);
    assert(!cpu.branch_pending);


    /* A not-taken BEQ still executes its delay slot, then continues. */
    const uint32_t beq_not_taken =
        (0x04u << 26) | (1u << 21) | (2u << 16) | 2u;
    memcpy(ram, &beq_not_taken, sizeof(beq_not_taken));
    memcpy(ram + 4, &nop, sizeof(nop));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 7u;
    cpu.gpr[2] = 8u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 4u);
    assert(!cpu.branch_pending);
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 8u);

    /* An exception in a branch delay slot sets BD and EPC to the branch. */
    memcpy(ram, &beq_r1_r1, sizeof(beq_r1_r1));
    memcpy(ram + 4, &reserved_special, sizeof(reserved_special));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 7u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 4u);
    assert(cpu.branch_pending);
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x80000000u) != 0);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (10u << 2));
    assert(cpu.cop0[14] == PSP_RAM_BASE);


    /* A taken BNE also executes its delay slot before applying the target. */
    const uint32_t bne_r1_r2 =
        (0x05u << 26) | (1u << 21) | (2u << 16) | 3u;
    memcpy(ram, &bne_r1_r2, sizeof(bne_r1_r2));
    memcpy(ram + 4, &nop, sizeof(nop));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 1u;
    cpu.gpr[2] = 2u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 4u);
    assert(cpu.branch_pending);
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 16u);
    assert(!cpu.branch_pending);

    /* A JR register jump applies its target after the delay slot. */
    const uint32_t jr_r1 = (1u << 21) | 0x08u;
    memcpy(ram, &jr_r1, sizeof(jr_r1));
    memcpy(ram + 4, &nop, sizeof(nop));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = PSP_RAM_BASE + 12u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 4u);
    assert(cpu.branch_pending);
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 12u);
    assert(!cpu.branch_pending);

    return 0;
}
