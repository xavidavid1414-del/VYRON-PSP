#include "psp_interpreter.h"

#include <stddef.h>
#include <stdint.h>

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

bool psp_interpreter_step(PSP_CPU *cpu, PSP_Memory *memory)
{
    if (cpu == NULL || memory == NULL || cpu->halted)
        return false;

    const uint32_t pc = cpu->pc;
    const uint32_t instruction = psp_memory_read32(memory, pc);
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

    cpu->branch_pending = false;

    switch (opcode)
    {
        case 0x00:
            switch (funct)
            {
                case 0x00: set_register(cpu, rd, b << shamt); break;
                case 0x02: set_register(cpu, rd, b >> shamt); break;
                case 0x03: set_register(cpu, rd, arithmetic_shift_right(b, shamt)); break;
                case 0x04: set_register(cpu, rd, b << (a & 31u)); break;
                case 0x06: set_register(cpu, rd, b >> (a & 31u)); break;
                case 0x07: set_register(cpu, rd, arithmetic_shift_right(b, a)); break;

                case 0x08:
                    schedule_branch = true;
                    scheduled_target = a;
                    break;

                case 0x09:
                    set_register(cpu, rd == 0 ? 31u : rd, pc + 8u);
                    schedule_branch = true;
                    scheduled_target = a;
                    break;

                case 0x0C:
                    return false;

                case 0x10: set_register(cpu, rd, cpu->hi); break;
                case 0x11: cpu->hi = a; break;
                case 0x12: set_register(cpu, rd, cpu->lo); break;
                case 0x13: cpu->lo = a; break;

                case 0x18: {
                    int64_t v = (int64_t)(int32_t)a * (int64_t)(int32_t)b;
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
                        if (a == 0x80000000u && b == 0xFFFFFFFFu) {
                            cpu->lo = 0x80000000u;
                            cpu->hi = 0;
                        } else {
                            cpu->lo = (uint32_t)((int32_t)a / (int32_t)b);
                            cpu->hi = (uint32_t)((int32_t)a % (int32_t)b);
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
                case 0x21: set_register(cpu, rd, a + b); break;

                case 0x22:
                case 0x23: set_register(cpu, rd, a - b); break;

                case 0x24: set_register(cpu, rd, a & b); break;
                case 0x25: set_register(cpu, rd, a | b); break;
                case 0x26: set_register(cpu, rd, a ^ b); break;
                case 0x27: set_register(cpu, rd, ~(a | b)); break;
                case 0x2A: set_register(cpu, rd, (int32_t)a < (int32_t)b); break;
                case 0x2B: set_register(cpu, rd, a < b); break;

                default: return false;
            }
            break;

        case 0x01:
            if ((rt == 0u && (int32_t)a < 0) ||
                (rt == 1u && (int32_t)a >= 0) ||
                (rt == 16u && (int32_t)a < 0) ||
                (rt == 17u && (int32_t)a >= 0)) {
                schedule_branch = true;
                scheduled_target = pc + 4u + (sign_extend_16(immediate) << 2);
            }

            if (rt == 16u || rt == 17u)
                set_register(cpu, 31u, pc + 8u);

            if (rt != 0u && rt != 1u && rt != 16u && rt != 17u)
                return false;
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
                scheduled_target = pc + 4u + (sign_extend_16(immediate) << 2);
            }
            break;

        case 0x05:
            if (a != b) {
                schedule_branch = true;
                scheduled_target = pc + 4u + (sign_extend_16(immediate) << 2);
            }
            break;

        case 0x06:
            if ((int32_t)a <= 0) {
                schedule_branch = true;
                scheduled_target = pc + 4u + (sign_extend_16(immediate) << 2);
            }
            break;

        case 0x07:
            if ((int32_t)a > 0) {
                schedule_branch = true;
                scheduled_target = pc + 4u + (sign_extend_16(immediate) << 2);
            }
            break;

        case 0x08:
        case 0x09:
            set_register(cpu, rt, a + sign_extend_16(immediate));
            break;

        case 0x0A:
            set_register(cpu, rt, (int32_t)a < (int32_t)(int16_t)immediate);
            break;

        case 0x0B:
            set_register(cpu, rt, a < sign_extend_16(immediate));
            break;

        case 0x0C: set_register(cpu, rt, a & immediate); break;
        case 0x0D: set_register(cpu, rt, a | immediate); break;
        case 0x0E: set_register(cpu, rt, a ^ immediate); break;
        case 0x0F: set_register(cpu, rt, immediate << 16); break;

        case 0x10:
            if (rs == 0u)
                set_register(cpu, rt, cpu->cop0[rd]);
            else if (rs == 4u)
                cpu->cop0[rd] = b;
            else
                return false;
            break;

        case 0x20: {
            uint32_t address = a + sign_extend_16(immediate);
            set_register(cpu, rt,
                (uint32_t)(int32_t)(int8_t)psp_memory_read8(memory, address));
            break;
        }

        case 0x21: {
            uint32_t address = a + sign_extend_16(immediate);
            set_register(cpu, rt,
                (uint32_t)(int32_t)(int16_t)read16(memory, address));
            break;
        }

        case 0x23: {
            uint32_t address = a + sign_extend_16(immediate);
            set_register(cpu, rt, psp_memory_read32(memory, address));
            break;
        }

        case 0x24: {
            uint32_t address = a + sign_extend_16(immediate);
            set_register(cpu, rt, psp_memory_read8(memory, address));
            break;
        }

        case 0x25: {
            uint32_t address = a + sign_extend_16(immediate);
            set_register(cpu, rt, read16(memory, address));
            break;
        }

        case 0x28: {
            uint32_t address = a + sign_extend_16(immediate);
            psp_memory_write8(memory, address, (uint8_t)b);
            break;
        }

        case 0x29: {
            uint32_t address = a + sign_extend_16(immediate);
            write16(memory, address, b);
            break;
        }

        case 0x2B: {
            uint32_t address = a + sign_extend_16(immediate);
            psp_memory_write32(memory, address, b);
            break;
        }

        default:
            return false;
    }

    if (had_pending_branch) {
        next_pc = old_branch_target;
    } else if (schedule_branch) {
        cpu->branch_pending = true;
        cpu->branch_target = scheduled_target;
    }

    cpu->pc = next_pc;
    cpu->gpr[0] = 0;

    return true;
}
