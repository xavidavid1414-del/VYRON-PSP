#include "psp_interpreter.h"
#include "psp_syscall.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define COP0_BADVADDR 8u
#define COP0_CAUSE 13u
#define EXCEPTION_VECTOR (PSP_RAM_BASE + 0x180u)

static bool test_syscall_handler(
    PSP_CPU *cpu,
    PSP_Memory *memory,
    uint32_t syscall_code
)
{
    (void)memory;

    if (syscall_code != 0x54321u)
        return false;

    cpu->gpr[2] = 0xC0DEF00Du;
    return true;
}

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

    /* ADD signed overflow must raise OV without changing rd. */
    const uint32_t add_overflow =
        (1u << 21) | (2u << 16) | (3u << 11) | 0x20u;
    memcpy(ram, &add_overflow, sizeof(add_overflow));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 0x7FFFFFFFu;
    cpu.gpr[2] = 1u;
    cpu.gpr[3] = 0x13579BDFu;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (12u << 2));
    assert(cpu.cop0[14] == PSP_RAM_BASE);
    assert(cpu.gpr[3] == 0x13579BDFu);

    /* SUB signed overflow must raise OV without changing rd. */
    const uint32_t sub_overflow =
        (1u << 21) | (2u << 16) | (3u << 11) | 0x22u;
    memcpy(ram, &sub_overflow, sizeof(sub_overflow));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 0x80000000u;
    cpu.gpr[2] = 1u;
    cpu.gpr[3] = 0x2468ACE0u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (12u << 2));
    assert(cpu.cop0[14] == PSP_RAM_BASE);
    assert(cpu.gpr[3] == 0x2468ACE0u);

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


    /* BLTZAL must not write the link register when not taken. */
    const uint32_t bltzal_not_taken =
        (0x01u << 26) | (1u << 21) | (16u << 16) | 2u;
    memcpy(ram, &bltzal_not_taken, sizeof(bltzal_not_taken));
    memcpy(ram + 4, &nop, sizeof(nop));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 1u;
    cpu.gpr[31] = 0x12345678u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 4u);
    assert(!cpu.branch_pending);
    assert(cpu.gpr[31] == 0x12345678u);
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 8u);

    /* BGEZAL writes the link register when its branch is taken. */
    const uint32_t bgezal_taken =
        (0x01u << 26) | (1u << 21) | (17u << 16) | 2u;
    memcpy(ram, &bgezal_taken, sizeof(bgezal_taken));
    memcpy(ram + 4, &nop, sizeof(nop));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 1u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 4u);
    assert(cpu.branch_pending);
    assert(cpu.gpr[31] == PSP_RAM_BASE + 8u);
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 12u);
    assert(!cpu.branch_pending);

    /* JAL writes the return address and executes its delay slot. */
    const uint32_t jal = (0x03u << 26) |
        (((PSP_RAM_BASE + 16u) >> 2) & 0x03FFFFFFu);
    memcpy(ram, &jal, sizeof(jal));
    memcpy(ram + 4, &nop, sizeof(nop));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 4u);
    assert(cpu.gpr[31] == PSP_RAM_BASE + 8u);
    assert(cpu.branch_pending);
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 16u);
    assert(!cpu.branch_pending);

    /* JALR writes rd (or r31 when rd is zero) before the delay slot. */
    const uint32_t jalr_r1_r5 =
        (1u << 21) | (5u << 11) | 0x09u;
    memcpy(ram, &jalr_r1_r5, sizeof(jalr_r1_r5));
    memcpy(ram + 4, &nop, sizeof(nop));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = PSP_RAM_BASE + 12u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 4u);
    assert(cpu.gpr[5] == PSP_RAM_BASE + 8u);
    assert(cpu.branch_pending);
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 12u);
    assert(!cpu.branch_pending);


    /* Signed ADD overflow in a branch delay slot must set BD and EPC. */
    memcpy(ram, &beq_r1_r1, sizeof(beq_r1_r1));
    memcpy(ram + 4, &add_overflow, sizeof(add_overflow));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 0x7FFFFFFFu;
    cpu.gpr[2] = 1u;
    cpu.gpr[3] = 0xAABBCCDDu;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.branch_pending);
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x80000000u) != 0);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (12u << 2));
    assert(cpu.cop0[14] == PSP_RAM_BASE);
    assert(cpu.gpr[3] == 0xAABBCCDDu);

    /* A fetch fault in a branch delay slot must also set BD and EPC. */
    const uint32_t branch_at_last_word =
        (0x04u << 26) | (1u << 21) | (1u << 16) | 1u;
    memcpy(ram + 12, &branch_at_last_word, sizeof(branch_at_last_word));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE + 12u;
    cpu.gpr[1] = 1u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 16u);
    assert(cpu.branch_pending);
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x80000000u) != 0);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (4u << 2));
    assert(cpu.cop0[14] == PSP_RAM_BASE + 12u);
    assert(cpu.cop0[COP0_BADVADDR] == PSP_RAM_BASE + 16u);

    /* A registered SYSCALL is dispatched and completes normally. */
    const uint32_t handled_syscall =
        (0x54321u << 6) | 0x0Cu;
    memcpy(ram, &handled_syscall, sizeof(handled_syscall));

    psp_syscall_reset();
    assert(psp_syscall_register(0x54321u, test_syscall_handler));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 4u);
    assert(cpu.gpr[2] == 0xC0DEF00Du);

    /* An unregistered SYSCALL raises the system-call exception. */
    psp_syscall_reset();
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[2] = 0x13572468u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == EXCEPTION_VECTOR);
    assert((cpu.cop0[COP0_CAUSE] & 0x7Cu) == (8u << 2));
    assert(cpu.cop0[14] == PSP_RAM_BASE);
    assert(cpu.gpr[2] == 0x13572468u);

    /* A handled syscall in a branch delay slot preserves branch flow. */
    const uint32_t branch_then_syscall =
        (0x04u << 26) | (1u << 21) | (1u << 16) | 2u;
    memcpy(ram, &branch_then_syscall, sizeof(branch_then_syscall));
    memcpy(ram + 4, &handled_syscall, sizeof(handled_syscall));

    psp_syscall_reset();
    assert(psp_syscall_register(0x54321u, test_syscall_handler));

    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 9u;

    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.branch_pending);
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.pc == PSP_RAM_BASE + 12u);
    assert(!cpu.branch_pending);
    assert(cpu.gpr[2] == 0xC0DEF00Du);

    psp_syscall_reset();

    /* MULT stores the signed 64-bit product in HI:LO. */
    const uint32_t mult_signed =
        (1u << 21) | (2u << 16) | 0x18u;
    memcpy(ram, &mult_signed, sizeof(mult_signed));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = (uint32_t)-7;
    cpu.gpr[2] = 9u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.lo == 0xFFFFFFC1u);
    assert(cpu.hi == 0xFFFFFFFFu);

    /* MULTU stores the unsigned 64-bit product in HI:LO. */
    const uint32_t mult_unsigned =
        (1u << 21) | (2u << 16) | 0x19u;
    memcpy(ram, &mult_unsigned, sizeof(mult_unsigned));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 0xFFFFFFFFu;
    cpu.gpr[2] = 2u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.lo == 0xFFFFFFFEu);
    assert(cpu.hi == 1u);

    /* DIV truncates toward zero and keeps the signed remainder in HI. */
    const uint32_t div_signed =
        (1u << 21) | (2u << 16) | 0x1Au;
    memcpy(ram, &div_signed, sizeof(div_signed));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = (uint32_t)-17;
    cpu.gpr[2] = 5u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.lo == (uint32_t)-3);
    assert(cpu.hi == (uint32_t)-2);

    /* DIVU stores quotient in LO and remainder in HI. */
    const uint32_t div_unsigned =
        (1u << 21) | (2u << 16) | 0x1Bu;
    memcpy(ram, &div_unsigned, sizeof(div_unsigned));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 17u;
    cpu.gpr[2] = 5u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.lo == 3u);
    assert(cpu.hi == 2u);

    /* Signed division's INT_MIN / -1 overflow case is deterministic. */
    memcpy(ram, &div_signed, sizeof(div_signed));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 0x80000000u;
    cpu.gpr[2] = 0xFFFFFFFFu;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.lo == 0x80000000u);
    assert(cpu.hi == 0u);

    /* DIV by zero preserves the previous HI:LO values. */
    memcpy(ram, &div_signed, sizeof(div_signed));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 123u;
    cpu.gpr[2] = 0u;
    cpu.hi = 0x13579BDFu;
    cpu.lo = 0x2468ACE0u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.lo == 0x2468ACE0u);
    assert(cpu.hi == 0x13579BDFu);

    /* DIVU by zero also preserves HI:LO. */
    memcpy(ram, &div_unsigned, sizeof(div_unsigned));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 123u;
    cpu.gpr[2] = 0u;
    cpu.hi = 0x89ABCDEFu;
    cpu.lo = 0x01234567u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.lo == 0x01234567u);
    assert(cpu.hi == 0x89ABCDEFu);

    /* SPECIAL2 MADD accumulates a signed product into HI:LO. */
    const uint32_t madd = (0x1Cu << 26) | (1u << 21) | (2u << 16) | 0x00u;
    memcpy(ram, &madd, sizeof(madd));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = (uint32_t)-3;
    cpu.gpr[2] = 4u;
    cpu.lo = 10u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.hi == 0xFFFFFFFFu);
    assert(cpu.lo == 0xFFFFFFFEu);

    /* SPECIAL2 MADDU accumulates an unsigned product. */
    const uint32_t maddu = (0x1Cu << 26) | (1u << 21) | (2u << 16) | 0x01u;
    memcpy(ram, &maddu, sizeof(maddu));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 0xFFFFFFFFu;
    cpu.gpr[2] = 2u;
    cpu.lo = 1u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.hi == 1u);
    assert(cpu.lo == 0xFFFFFFFFu);

    /* SPECIAL2 MSUB subtracts a signed product from HI:LO. */
    const uint32_t msub = (0x1Cu << 26) | (1u << 21) | (2u << 16) | 0x04u;
    memcpy(ram, &msub, sizeof(msub));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = (uint32_t)-3;
    cpu.gpr[2] = 4u;
    cpu.lo = 10u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.hi == 0u);
    assert(cpu.lo == 22u);

    /* SPECIAL2 MSUBU subtracts an unsigned product modulo 64 bits. */
    const uint32_t msubu = (0x1Cu << 26) | (1u << 21) | (2u << 16) | 0x05u;
    memcpy(ram, &msubu, sizeof(msubu));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 3u;
    cpu.gpr[2] = 4u;
    cpu.lo = 10u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.hi == 0xFFFFFFFFu);
    assert(cpu.lo == 0xFFFFFFFEu);

    /* SPECIAL2 MUL writes the low signed product to rd. */
    const uint32_t mul = (0x1Cu << 26) | (1u << 21) | (2u << 16) |
                         (3u << 11) | 0x02u;
    memcpy(ram, &mul, sizeof(mul));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = (uint32_t)-7;
    cpu.gpr[2] = 9u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.gpr[3] == 0xFFFFFFC1u);

    /* SPECIAL2 CLZ and CLO count leading zeroes and ones. */
    const uint32_t clz = (0x1Cu << 26) | (1u << 21) | (3u << 11) | 0x20u;
    memcpy(ram, &clz, sizeof(clz));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 0x00F00000u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.gpr[3] == 8u);

    const uint32_t clo = (0x1Cu << 26) | (1u << 21) | (3u << 11) | 0x21u;
    memcpy(ram, &clo, sizeof(clo));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 0xF0FFFFFFu;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.gpr[3] == 4u);

    /* Allegrex MAX and MIN use signed 32-bit comparisons. */
    const uint32_t max_inst = (1u << 21) | (2u << 16) |
                             (3u << 11) | 0x2Cu;
    memcpy(ram, &max_inst, sizeof(max_inst));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 0xFFFFFFF9u; /* -7 */
    cpu.gpr[2] = 9u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.gpr[3] == 9u);

    const uint32_t min_inst = (1u << 21) | (2u << 16) |
                             (3u << 11) | 0x2Du;
    memcpy(ram, &min_inst, sizeof(min_inst));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 0xFFFFFFF9u; /* -7 */
    cpu.gpr[2] = 9u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.gpr[3] == 0xFFFFFFF9u);

    /* MAX/MIN must treat unsigned-looking high-bit values as negative. */
    memcpy(ram, &max_inst, sizeof(max_inst));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 0x80000000u;
    cpu.gpr[2] = 0x7FFFFFFFu;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.gpr[3] == 0x7FFFFFFFu);

    /* Allegrex ROTR rotates right by the encoded immediate amount. */
    const uint32_t rotr = (1u << 21) | (2u << 16) |
                          (3u << 11) | (8u << 6) | 0x02u;
    memcpy(ram, &rotr, sizeof(rotr));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[2] = 0x12345678u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.gpr[3] == 0x78123456u);

    /* ROTR by zero leaves the value unchanged. */
    const uint32_t rotr_zero = (1u << 21) | (2u << 16) |
                               (3u << 11) | 0x02u;
    memcpy(ram, &rotr_zero, sizeof(rotr_zero));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[2] = 0x89ABCDEFu;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.gpr[3] == 0x89ABCDEFu);

    /* ROTRV uses rs[4:0] as its variable rotate amount. */
    const uint32_t rotrv = (1u << 21) | (2u << 16) |
                           (3u << 11) | (1u << 6) | 0x06u;
    memcpy(ram, &rotrv, sizeof(rotrv));
    psp_cpu_reset(&cpu);
    cpu.pc = PSP_RAM_BASE;
    cpu.gpr[1] = 12u;
    cpu.gpr[2] = 0x12345678u;
    assert(psp_interpreter_step(&cpu, &memory));
    assert(cpu.gpr[3] == 0x67812345u);

    /* SLLV and SRAV require the encoded shamt field to be zero. */
    const uint32_t reserved_variable_shifts[] = {
        (1u << 21) | (2u << 16) | (3u << 11) | (1u << 6) | 0x04u,
        (1u << 21) | (2u << 16) | (3u << 11) | (1u << 6) | 0x07u
    };
    for (size_t i = 0; i < sizeof(reserved_variable_shifts) /
                            sizeof(reserved_variable_shifts[0]); ++i) {
        memcpy(ram, &reserved_variable_shifts[i],
               sizeof(reserved_variable_shifts[i]));
        psp_cpu_reset(&cpu);
        cpu.pc = PSP_RAM_BASE;
        cpu.gpr[1] = 4u;
        cpu.gpr[2] = 0x12345678u;
        assert(psp_interpreter_step(&cpu, &memory));
        assert(cpu.pc == PSP_RAM_BASE + 0x180u);
        assert((cpu.cop0[13] & 0x7Cu) == (10u << 2));
    }

    /* SLL and SRA require rs=0; malformed encodings raise RI. */
    const uint32_t invalid_fixed_shifts[] = {
        (2u << 21) | (3u << 11),
        (2u << 21) | (3u << 11) | (4u << 6) | 0x03u
    };
    for (size_t i = 0; i < sizeof(invalid_fixed_shifts) /
                            sizeof(invalid_fixed_shifts[0]); ++i) {
        memcpy(ram, &invalid_fixed_shifts[i], sizeof(invalid_fixed_shifts[i]));
        psp_cpu_reset(&cpu);
        cpu.pc = PSP_RAM_BASE;
        cpu.gpr[2] = 0x12345678u;
        assert(psp_interpreter_step(&cpu, &memory));
        assert(cpu.pc == PSP_RAM_BASE + 0x180u);
        assert((cpu.cop0[13] & 0x7Cu) == (10u << 2));
    }

    return 0;
}
