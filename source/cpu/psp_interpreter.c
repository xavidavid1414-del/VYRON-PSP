#include "psp_interpreter.h"

static uint32_t get_register(PSP_CPU *cpu, uint32_t index)
{
    return index == 0 ? 0 : cpu->gpr[index];
}

static void set_register(PSP_CPU *cpu, uint32_t index, uint32_t value)
{
    if (index != 0)
        cpu->gpr[index] = value;
}

bool psp_interpreter_step(PSP_CPU *cpu, PSP_Memory *memory)
{
    if (cpu == NULL || memory == NULL || cpu->halted)
        return false;

    uint32_t instruction = psp_memory_read32(memory, cpu->pc);

    uint32_t opcode = instruction >> 26;
    uint32_t rs = (instruction >> 21) & 0x1F;
    uint32_t rt = (instruction >> 16) & 0x1F;
    uint32_t rd = (instruction >> 11) & 0x1F;
    uint32_t funct = instruction & 0x3F;
    int16_t immediate = (int16_t)(instruction & 0xFFFF);

    uint32_t next_pc = cpu->pc + 4;
    uint32_t a = get_register(cpu, rs);
    uint32_t b = get_register(cpu, rt);

    switch (opcode)
    {
        case 0x00:
            switch (funct)
            {
                case 0x00:
                    set_register(cpu, rd, b << ((instruction >> 6) & 0x1F));
                    break;

                case 0x02:
                    set_register(cpu, rd, b >> ((instruction >> 6) & 0x1F));
                    break;

                case 0x08:
                    next_pc = a;
                    break;

                case 0x21:
                    set_register(cpu, rd, a + b);
                    break;

                case 0x23:
                    set_register(cpu, rd, a - b);
                    break;

                default:
                    return false;
            }
            break;

        case 0x08:
            set_register(cpu, rt, a + (int32_t)immediate);
            break;

        case 0x0C:
            set_register(cpu, rt, a & (uint16_t)immediate);
            break;

        case 0x0D:
            set_register(cpu, rt, a | (uint16_t)immediate);
            break;

        case 0x0F:
            set_register(cpu, rt, (uint32_t)(uint16_t)immediate << 16);
            break;

        case 0x23:
        {
            uint32_t address = a + (int32_t)immediate;
            set_register(cpu, rt, psp_memory_read32(memory, address));
            break;
        }

        case 0x2B:
        {
            uint32_t address = a + (int32_t)immediate;
            psp_memory_write32(memory, address, b);
            break;
        }

        case 0x04:
            if (a == b)
                next_pc += (int32_t)immediate * 4;
            break;

        case 0x05:
            if (a != b)
                next_pc += (int32_t)immediate * 4;
            break;

        default:
            return false;
    }

    cpu->pc = next_pc;
    cpu->gpr[0] = 0;

    return true;
}
