
#include "psp_interpreter.h"
#include "psp_syscall.h"

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#define COP0_BADVADDR 8u
#define COP0_STATUS   12u
#define COP0_CAUSE    13u
#define COP0_EPC      14u

#define EXC_ADEL 4u
#define EXC_ADES 5u
#define EXC_SYS  8u
#define EXC_BP   9u
#define EXC_RI   10u
#define EXC_OV   12u

#define EXCEPTION_VECTOR (PSP_RAM_BASE + 0x180u)
#define STATUS_EXL 0x00000002u
#define CAUSE_BD   0x80000000u
#define CAUSE_EXCCODE_MASK 0x0000007Cu

static uint32_t get_register(const PSP_CPU *cpu, uint32_t index)
{
    return index == 0 ? 0 : cpu->gpr[index & 31u];
}

static void set_register(PSP_CPU *cpu, uint32_t index, uint32_t value)
{
    if ((index & 31u) != 0)
        cpu->gpr[index & 31u] = value;
}

static uint32_t sign_extend_16(uint32_t value)
{
    return (uint32_t)(int32_t)(int16_t)(value & 0xFFFFu);
}

static uint32_t arithmetic_shift_right(uint32_t value, uint32_t amount)
{
    amount &= 31u;

    if (amount == 0)
        return value;

    uint32_t shifted = value >> amount;

    if ((value & 0x80000000u) != 0)
        shifted |= (~0u << (32u - amount));

    return shifted;
}

static uint32_t read16(PSP_Memory *memory, uint32_t address)
{
    return (uint32_t)psp_memory_read8(memory, address)
         | ((uint32_t)psp_memory_read8(memory, address + 1u) << 8);
}

static void write16(PSP_Memory *memory, uint32_t address, uint32_t value)
{
    psp_memory_write8(memory, address, (uint8_t)value);
    psp_memory_write8(memory, address + 1u, (uint8_t)(value >> 8));
}

static void enter_exception(
    PSP_CPU *cpu,
    uint32_t pc,
    bool in_delay_slot,
    uint32_t exception_code,
    uint32_t bad_address,
    bool has_bad_address)
{
    if ((cpu->cop0[COP0_STATUS] & STATUS_EXL) == 0) {
        cpu->cop0[COP0_EPC] =
            in_delay_slot ? pc - 4u : pc;

        uint32_t cause = cpu->cop0[COP0_CAUSE];
        cause &= ~CAUSE_BD;

        if (in_delay_slot)
            cause |= CAUSE_BD;

        cpu->cop0[COP0_CAUSE] = cause;
    }

    cpu->cop0[COP0_CAUSE] &= ~CAUSE_EXCCODE_MASK;
    cpu->cop0[COP0_CAUSE] |= exception_code << 2;

    if (has_bad_address)
        cpu->cop0[COP0_BADVADDR] = bad_address;

    cpu->cop0[COP0_STATUS] |= STATUS_EXL;

    cpu->branch_pending = false;
    cpu->pc = EXCEPTION_VECTOR;
    cpu->gpr[0] = 0;
}

static bool signed_add_overflow(
    uint32_t a,
    uint32_t b,
    uint32_t *result)
{
    int64_t value =
        (int64_t)(int32_t)a + (int64_t)(int32_t)b;

    if (value > INT32_MAX || value < INT32_MIN)
        return true;

    *result = (uint32_t)(int32_t)value;
    return false;
}

static bool signed_sub_overflow(
    uint32_t a,
    uint32_t b,
    uint32_t *result)
{
    int64_t value =
        (int64_t)(int32_t)a - (int64_t)(int32_t)b;

    if (value > INT32_MAX || value < INT32_MIN)
        return true;

    *result = (uint32_t)(int32_t)value;
    return false;
}

bool psp_interpreter_execute_instruction(PSP_CPU *cpu, PSP_Memory *memory,
                                         uint32_t instruction)
{
    if (cpu == NULL || memory == NULL || cpu->halted)
        return false;
    const uint32_t pc = cpu->pc;

    const uint32_t opcode = instruction >> 26;
    const uint32_t rs = (instruction >> 21) & 31u;
    const uint32_t rt = (instruction >> 16) & 31u;
    const uint32_t rd = (instruction >> 11) & 31u;
    const uint32_t shamt = (instruction >> 6) & 31u;
    const uint32_t funct = instruction & 63u;
    const uint32_t immediate = instruction & 0xFFFFu;

    const uint32_t a = get_register(cpu, rs);
    const uint32_t b = get_register(cpu, rt);

    const bool had_pending_branch = cpu->branch_pending;
    const uint32_t old_branch_target = cpu->branch_target;

    bool schedule_branch = false;
    uint32_t scheduled_target = 0;
    uint32_t next_pc = pc + 4u;
    uint32_t value = 0;

    cpu->branch_pending = false;

    switch (opcode)
    {
        case 0x22: { /* LWL */
            uint32_t address = a + sign_extend_16(immediate);
            uint32_t aligned_address = address & ~3u;

            if (!psp_memory_is_valid(memory, aligned_address, 4)) {
                enter_exception(
                    cpu, pc, had_pending_branch,
                    EXC_ADEL, address, true);
                return true;
            }
            uint32_t shift = (address & 3u) * 8u;
            uint32_t word = psp_memory_read32(
                memory, address & ~3u);

            value = (b & (0x00FFFFFFu >> shift))
                  | (word << (24u - shift));

            set_register(cpu, rt, value);
            break;
        }

        case 0x26: { /* LWR */
            uint32_t address = a + sign_extend_16(immediate);
            uint32_t aligned_address = address & ~3u;

            if (!psp_memory_is_valid(memory, aligned_address, 4)) {
                enter_exception(
                    cpu, pc, had_pending_branch,
                    EXC_ADEL, address, true);
                return true;
            }
            uint32_t shift = (address & 3u) * 8u;
            uint32_t word = psp_memory_read32(
                memory, address & ~3u);

            value = (b & (0xFFFFFF00u << (24u - shift)))
                  | (word >> shift);

            set_register(cpu, rt, value);
            break;
        }

        case 0x2A: { /* SWL */
            uint32_t address = a + sign_extend_16(immediate);
            uint32_t aligned_address = address & ~3u;

            if (!psp_memory_is_valid(memory, aligned_address, 4)) {
                enter_exception(
                    cpu, pc, had_pending_branch,
                    EXC_ADES, address, true);
                return true;
            }
            uint32_t shift = (address & 3u) * 8u;
            uint32_t mask = 0xFFFFFFFFu >> (24u - shift);
            uint32_t aligned = address & ~3u;
            uint32_t old = psp_memory_read32(memory, aligned);

            value = (old & ~mask) |
                    ((b >> (24u - shift)) & mask);

            psp_memory_write32(memory, aligned, value);
            break;
        }

        case 0x2E: { /* SWR */
            uint32_t address = a + sign_extend_16(immediate);
            uint32_t aligned_address = address & ~3u;

            if (!psp_memory_is_valid(memory, aligned_address, 4)) {
                enter_exception(
                    cpu, pc, had_pending_branch,
                    EXC_ADES, address, true);
                return true;
            }
            uint32_t shift = (address & 3u) * 8u;
            uint32_t mask = 0xFFFFFFFFu << shift;
            uint32_t aligned = address & ~3u;
            uint32_t old = psp_memory_read32(memory, aligned);

            value = (old & ~mask) | ((b << shift) & mask);

            psp_memory_write32(memory, aligned, value);
            break;
        }

        case 0x00:
            switch (funct)
            {
                case 0x00:
                    set_register(cpu, rd, b << shamt);
                    break;

                case 0x02:
                    set_register(cpu, rd, b >> shamt);
                    break;

                case 0x03:
                    set_register(cpu, rd,
                        arithmetic_shift_right(b, shamt));
                    break;

                case 0x04:
                    set_register(cpu, rd, b << (a & 31u));
                    break;

                case 0x06:
                    set_register(cpu, rd, b >> (a & 31u));
                    break;

                case 0x07:
                    set_register(cpu, rd,
                        arithmetic_shift_right(b, a));
                    break;

                case 0x08:
                    schedule_branch = true;
                    scheduled_target = a;
                    break;

                case 0x09:
                    set_register(cpu, rd == 0 ? 31u : rd, pc + 8u);
                    schedule_branch = true;
                    scheduled_target = a;
                    break;

                case 0x0A: /* MOVZ */
                    if (b == 0)
                        set_register(cpu, rd, a);
                    break;

                case 0x0B: /* MOVN */
                    if (b != 0)
                        set_register(cpu, rd, a);
                    break;

                case 0x0C: {
                    uint32_t syscall_code =
                        (instruction >> 6) & 0xFFFFFu;

                    if (!psp_syscall_dispatch(
                            cpu, memory, syscall_code)) {
                        enter_exception(
                            cpu, pc, had_pending_branch,
                            EXC_SYS, 0, false);
                        return true;
                    }

                    /* A handled syscall completes like a normal instruction. */
                    break;
                }

                case 0x0D:
                    enter_exception(
                        cpu, pc, had_pending_branch,
                        EXC_BP, 0, false);
                    return true;

                case 0x0F: /* SYNC: ordering is implicit in this single-threaded core. */
                    break;

                case 0x10:
                    set_register(cpu, rd, cpu->hi);
                    break;

                case 0x11:
                    cpu->hi = a;
                    break;

                case 0x12:
                    set_register(cpu, rd, cpu->lo);
                    break;

                case 0x13:
                    cpu->lo = a;
                    break;

                case 0x18: {
                    int64_t v =
                        (int64_t)(int32_t)a *
                        (int64_t)(int32_t)b;

                    cpu->lo = (uint32_t)v;
                    cpu->hi = (uint32_t)((uint64_t)v >> 32);
                    break;
                }

                case 0x19: {
                    uint64_t v = (uint64_t)a * (uint64_t)b;

                    cpu->lo = (uint32_t)v;
                    cpu->hi = (uint32_t)(v >> 32);
                    break;
                }

                case 0x1A:
                    if ((int32_t)b != 0) {
                        if (a == 0x80000000u &&
                            b == 0xFFFFFFFFu) {
                            cpu->lo = 0x80000000u;
                            cpu->hi = 0;
                        } else {
                            cpu->lo =
                                (uint32_t)((int32_t)a / (int32_t)b);
                            cpu->hi =
                                (uint32_t)((int32_t)a % (int32_t)b);
                        }
                    }
                    break;

                case 0x1B:
                    if (b != 0) {
                        cpu->lo = a / b;
                        cpu->hi = a % b;
                    }
                    break;

                case 0x20:
                    if (signed_add_overflow(a, b, &value)) {
                        enter_exception(
                            cpu, pc, had_pending_branch,
                            EXC_OV, 0, false);
                        return true;
                    }
                    set_register(cpu, rd, value);
                    break;

                case 0x21:
                    set_register(cpu, rd, a + b);
                    break;

                case 0x22:
                    if (signed_sub_overflow(a, b, &value)) {
                        enter_exception(
                            cpu, pc, had_pending_branch,
                            EXC_OV, 0, false);
                        return true;
                    }
                    set_register(cpu, rd, value);
                    break;

                case 0x23:
                    set_register(cpu, rd, a - b);
                    break;

                case 0x24:
                    set_register(cpu, rd, a & b);
                    break;

                case 0x25:
                    set_register(cpu, rd, a | b);
                    break;

                case 0x26:
                    set_register(cpu, rd, a ^ b);
                    break;

                case 0x27:
                    set_register(cpu, rd, ~(a | b));
                    break;

                case 0x2A:
                    set_register(cpu, rd,
                        (int32_t)a < (int32_t)b);
                    break;

                case 0x2B:
                    set_register(cpu, rd, a < b);
                    break;

                default:
                    enter_exception(
                        cpu, pc, had_pending_branch,
                        EXC_RI, 0, false);
                    return true;
            }
            break;

        case 0x01:
            bool branch_condition =
                (rt == 0u && (int32_t)a < 0) ||
                (rt == 1u && (int32_t)a >= 0) ||
                (rt == 16u && (int32_t)a < 0) ||
                (rt == 17u && (int32_t)a >= 0);

            if (branch_condition) {
                schedule_branch = true;
                scheduled_target =
                    pc + 4u + (sign_extend_16(immediate) << 2);

                if (rt == 16u || rt == 17u)
                    set_register(cpu, 31u, pc + 8u);
            }

            if (rt != 0u && rt != 1u &&
                rt != 16u && rt != 17u) {
                enter_exception(
                    cpu, pc, had_pending_branch,
                    EXC_RI, 0, false);
                return true;
            }
            break;

        case 0x02:
        case 0x03:
            if (opcode == 0x03)
                set_register(cpu, 31u, pc + 8u);

            schedule_branch = true;
            scheduled_target =
                ((pc + 4u) & 0xF0000000u) |
                ((instruction & 0x03FFFFFFu) << 2);
            break;

        case 0x04:
            if (a == b) {
                schedule_branch = true;
                scheduled_target =
                    pc + 4u + (sign_extend_16(immediate) << 2);
            }
            break;

        case 0x05:
            if (a != b) {
                schedule_branch = true;
                scheduled_target =
                    pc + 4u + (sign_extend_16(immediate) << 2);
            }
            break;

        case 0x06:
            if ((int32_t)a <= 0) {
                schedule_branch = true;
                scheduled_target =
                    pc + 4u + (sign_extend_16(immediate) << 2);
            }
            break;

        case 0x07:
            if ((int32_t)a > 0) {
                schedule_branch = true;
                scheduled_target =
                    pc + 4u + (sign_extend_16(immediate) << 2);
            }
            break;

        case 0x08:
            if (signed_add_overflow(
                    a, sign_extend_16(immediate), &value)) {
                enter_exception(
                    cpu, pc, had_pending_branch,
                    EXC_OV, 0, false);
                return true;
            }
            set_register(cpu, rt, value);
            break;

        case 0x09:
            set_register(cpu, rt,
                a + sign_extend_16(immediate));
            break;

        case 0x0A:
            set_register(cpu, rt,
                (int32_t)a < (int32_t)(int16_t)immediate);
            break;

        case 0x0B:
            set_register(cpu, rt,
                a < sign_extend_16(immediate));
            break;

        case 0x0C:
            set_register(cpu, rt, a & immediate);
            break;

        case 0x0D:
            set_register(cpu, rt, a | immediate);
            break;

        case 0x0E:
            set_register(cpu, rt, a ^ immediate);
            break;

        case 0x0F:
            set_register(cpu, rt, immediate << 16);
            break;

        case 0x10:
            if (rs == 0u) {
                /* MFC0 */
                set_register(cpu, rt, cpu->cop0[rd]);
            } else if (rs == 4u) {
                /* MTC0 */
                cpu->cop0[rd] = b;
            } else if (rs == 16u && funct == 0x18u) {
                /* ERET */
                if (had_pending_branch) {
                    enter_exception(
                        cpu, pc, true, EXC_RI, 0, false);
                    return true;
                }

                cpu->pc = cpu->cop0[COP0_EPC];
                cpu->cop0[COP0_STATUS] &= ~STATUS_EXL;
                cpu->branch_pending = false;
                cpu->gpr[0] = 0;
                return true;
            } else {
                enter_exception(
                    cpu, pc, had_pending_branch,
                    EXC_RI, 0, false);
                return true;
            }
            break;

        case 0x1C: /* SPECIAL2 */
            switch (funct)
            {
                case 0x00: /* MADD */
                case 0x04: { /* MSUB */
                    int64_t product =
                        (int64_t)(int32_t)a *
                        (int64_t)(int32_t)b;

                    uint64_t accumulator =
                        ((uint64_t)cpu->hi << 32) | cpu->lo;

                    if (funct == 0x00)
                        accumulator += (uint64_t)product;
                    else
                        accumulator -= (uint64_t)product;

                    cpu->lo = (uint32_t)accumulator;
                    cpu->hi = (uint32_t)(accumulator >> 32);
                    break;
                }

                case 0x01: /* MADDU */
                case 0x05: { /* MSUBU */
                    uint64_t product = (uint64_t)a * b;
                    uint64_t accumulator =
                        ((uint64_t)cpu->hi << 32) | cpu->lo;

                    if (funct == 0x01)
                        accumulator += product;
                    else
                        accumulator -= product;

                    cpu->lo = (uint32_t)accumulator;
                    cpu->hi = (uint32_t)(accumulator >> 32);
                    break;
                }

                case 0x02: /* MUL */
                    set_register(cpu, rd,
                        (uint32_t)((int64_t)(int32_t)a *
                                   (int64_t)(int32_t)b));
                    break;

                case 0x20: /* CLZ */
                case 0x21: { /* CLO */
                    uint32_t count = 0;
                    uint32_t bit = 0x80000000u;
                    bool count_ones = (funct == 0x21);

                    while (bit != 0 &&
                           (((a & bit) != 0) == count_ones)) {
                        count++;
                        bit >>= 1;
                    }

                    set_register(cpu, rd, count);
                    break;
                }

                default:
                    enter_exception(
                        cpu, pc, had_pending_branch,
                        EXC_RI, 0, false);
                    return true;
            }
            break;

        case 0x20: {
            uint32_t address = a + sign_extend_16(immediate);

            if (!psp_memory_is_valid(memory, address, 1)) {
                enter_exception(cpu, pc, had_pending_branch, EXC_ADEL, address, true);
                return true;
            }

            set_register(
                cpu, rt,
                (uint32_t)(int32_t)(int8_t)
                psp_memory_read8(memory, address));
            break;
        }

        case 0x21:
        case 0x25: {
            uint32_t address = a + sign_extend_16(immediate);

            if ((address & 1u) != 0) {
                enter_exception(
                    cpu, pc, had_pending_branch,
                    EXC_ADEL, address, true);
                return true;
            }

            if (!psp_memory_is_valid(memory, address, 2)) {
                enter_exception(cpu, pc, had_pending_branch, EXC_ADEL, address, true);
                return true;
            }

            uint32_t loaded = read16(memory, address);

            if (opcode == 0x21)
                loaded = (uint32_t)(int32_t)(int16_t)loaded;

            set_register(cpu, rt, loaded);
            break;
        }

        case 0x23: {
            uint32_t address = a + sign_extend_16(immediate);

            if ((address & 3u) != 0 ||
                !psp_memory_is_valid(memory, address, 4)) {
                enter_exception(
                    cpu, pc, had_pending_branch,
                    EXC_ADEL, address, true);
                return true;
            }

            set_register(
                cpu, rt,
                psp_memory_read32(memory, address));
            break;
        }

        case 0x24: {
            uint32_t address = a + sign_extend_16(immediate);

            if (!psp_memory_is_valid(memory, address, 1)) {
                enter_exception(cpu, pc, had_pending_branch, EXC_ADEL, address, true);
                return true;
            }

            set_register(
                cpu, rt,
                psp_memory_read8(memory, address));
            break;
        }

        case 0x28: {
            uint32_t address = a + sign_extend_16(immediate);

            if (!psp_memory_is_valid(memory, address, 1)) {
                enter_exception(cpu, pc, had_pending_branch, EXC_ADES, address, true);
                return true;
            }

            psp_memory_write8(
                memory, address, (uint8_t)b);
            break;
        }

        case 0x29: {
            uint32_t address = a + sign_extend_16(immediate);

            if ((address & 1u) != 0) {
                enter_exception(
                    cpu, pc, had_pending_branch,
                    EXC_ADES, address, true);
                return true;
            }

            if (!psp_memory_is_valid(memory, address, 2)) {
                enter_exception(cpu, pc, had_pending_branch, EXC_ADES, address, true);
                return true;
            }

            write16(memory, address, b);
            break;
        }

        case 0x2B: {
            uint32_t address = a + sign_extend_16(immediate);

            if ((address & 3u) != 0) {
                enter_exception(
                    cpu, pc, had_pending_branch,
                    EXC_ADES, address, true);
                return true;
            }

            if (!psp_memory_is_valid(memory, address, 4)) {
                enter_exception(cpu, pc, had_pending_branch, EXC_ADES, address, true);
                return true;
            }

            psp_memory_write32(memory, address, b);
            break;
        }

        case 0x2F: /* CACHE: cache maintenance is not modeled yet. */
        case 0x33: /* PREF: memory prefetch is a performance hint. */
            break;

        default:
            enter_exception(
                cpu, pc, had_pending_branch,
                EXC_RI, 0, false);
            return true;
    }

        if (had_pending_branch)
        next_pc = old_branch_target;

    cpu->pc = next_pc;

    if (schedule_branch) {
        cpu->branch_pending = true;
        cpu->branch_target = scheduled_target;
    }

    cpu->gpr[0] = 0;

    return true;
}

bool psp_interpreter_step(PSP_CPU *cpu, PSP_Memory *memory)
{
    if (cpu == NULL || memory == NULL || cpu->halted)
        return false;
    const uint32_t pc = cpu->pc;
    if ((pc & 3u) != 0 || !psp_memory_is_valid(memory, pc, 4)) {
        enter_exception(cpu, pc, cpu->branch_pending, EXC_ADEL, pc, true);
        return true;
    }
    return psp_interpreter_execute_instruction(
        cpu, memory, psp_memory_read32(memory, pc));
}
