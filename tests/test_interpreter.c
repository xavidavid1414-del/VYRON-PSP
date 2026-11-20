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

    return 0;
}
